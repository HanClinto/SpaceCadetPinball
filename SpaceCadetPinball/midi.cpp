#include "pch.h"
#include "midi.h"


#include "pb.h"


std::vector<MusicHandle*> midi::LoadedTracks{};
MusicHandle* midi::track1, * midi::track2, * midi::track3;
MidiTracks midi::active_track, midi::NextTrack;
int midi::Volume = MIX_MAX_VOLUME;
bool midi::IsPlaying = false, midi::MixOpen = false;

#ifdef MUSIC_TSF
extern unsigned char gm_sf2[];
extern unsigned int gm_sf2_len;

tsf* midi::Synth = nullptr;
MusicHandle *midi::SongStart = nullptr, *midi::SongPos = nullptr;
double midi::SongTimeMs = 0, midi::MsPerSample = 1000.0 / MIX_DEFAULT_FREQUENCY;
#endif

constexpr uint32_t FOURCC(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	return static_cast<uint32_t>((d << 24) | (c << 16) | (b << 8) | a);
}

int ToVariableLen(uint32_t value, uint32_t& dst)
{
	auto count = 1;
	dst = value & 0x7F;

	while ((value >>= 7))
	{
		dst <<= 8;
		dst |= ((value & 0x7F) | 0x80);
		count++;
	}

	return count;
}

void midi::music_play()
{
	if (!IsPlaying)
	{
		IsPlaying = true;
		play_track(NextTrack, true);
		NextTrack = MidiTracks::None;
	}
}

void midi::music_stop()
{
	if (IsPlaying)
	{
		IsPlaying = false;
		NextTrack = active_track;
		StopPlayback();
	}
}

void midi::StopPlayback()
{
	if (active_track != MidiTracks::None)
	{
		if (MixOpen)
			HaltTrack();
		active_track = MidiTracks::None;
	}
}

int midi::music_init(bool mixOpen, int volume)
{
	MixOpen = mixOpen;
#ifdef MUSIC_TSF
	if (MixOpen && !Synth)
	{
		Synth = tsf_load_memory(gm_sf2, static_cast<int>(gm_sf2_len));
		int frequency = MIX_DEFAULT_FREQUENCY;
		Mix_QuerySpec(&frequency, nullptr, nullptr);
		tsf_set_output(Synth, TSF_STEREO_INTERLEAVED, frequency, 0.0f);
		MsPerSample = 1000.0 / frequency;
		Mix_HookMusic(TsfCallback, nullptr);
	}
#endif
	SetVolume(volume);
	active_track = MidiTracks::None;
	NextTrack = MidiTracks::None;
	IsPlaying = false;
	track1 = track2 = track3 = nullptr;

	if (pb::FullTiltMode)
	{
		track1 = load_track("TABA1");
		track2 = load_track("TABA2");
		track3 = load_track("TABA3");

		// FT demo .006 has only one music track, but it is nearly 9 min. long
		if (!track1 && pb::FullTiltDemoMode)
			track1 = load_track("DEMO");
	}
	else
	{
		// 3DPB has only one music track. PINBALL2.MID is a bitmap font, in the same format as PB_MSGFT.bin
		track1 = load_track("PINBALL");
	}

	return track1 != nullptr;
}

void midi::music_shutdown()
{
	music_stop();

	for (auto midi : LoadedTracks)
	{
		FreeTrack(midi);
	}
	active_track = MidiTracks::None;
	LoadedTracks.clear();
#ifdef MUSIC_TSF
	if (Synth)
	{
		Mix_HookMusic(nullptr, nullptr);
		tsf_close(Synth);
		Synth = nullptr;
	}
#endif
}

void midi::SetVolume(int volume)
{
	Volume = volume;
	if (MixOpen)
	{
#ifdef MUSIC_TSF
		if (Synth)
			tsf_set_volume(Synth, static_cast<float>(volume) / MIX_MAX_VOLUME);
#else
		Mix_VolumeMusic(volume);
#endif
	}
}

MusicHandle* midi::load_track(std::string fileName)
{
	if (!MixOpen || pb::quickFlag)
		return nullptr;

	if (pb::FullTiltMode)
	{
		// FT sounds are in SOUND subfolder
		fileName.insert(0, 1, PathSeparator);
		fileName.insert(0, "SOUND");
	}

	auto audio = load_track_sub(fileName, true);
	if (!audio)
		audio = load_track_sub(fileName, false);

	if (!audio)
		return nullptr;

	LoadedTracks.push_back(audio);
	return audio;
}

MusicHandle* midi::load_track_sub(std::string fileName, bool isMidi)
{
	// FT has music in two formats, depending on game version: MIDI in 16bit, MIDS in 32bit.
	// 3DPB music is MIDI only.
	MusicHandle* audio = nullptr;
	fileName += isMidi ? ".MID" : ".MDS";
	for (int i = 0; i < 2; i++)
	{
		if (i == 1)
			std::transform(fileName.begin(), fileName.end(), fileName.begin(),
			               [](unsigned char c) { return std::tolower(c); });
		if (isMidi)
		{
			auto filePath = pb::make_path_name(fileName);
			auto fileHandle = fopenu(filePath.c_str(), "rb");
			if (fileHandle)
			{
				fclose(fileHandle);
				auto rw = SDL_RWFromFile(filePath.c_str(), "rb");
				audio = LoadFromRW(rw);
				break;
			}
		}
		else
		{
			auto midi = MdsToMidi(pb::make_path_name(fileName));
			if (midi)
			{
				// Dump converted MIDI file
				/*auto filePath = fileName + ".midi";
				FILE* fileHandle = fopenu(filePath.c_str(), "wb");
				fwrite(midi->data(), 1, midi->size(), fileHandle);
				fclose(fileHandle);*/

				auto rw = SDL_RWFromMem(midi->data(), static_cast<int>(midi->size()));
				audio = LoadFromRW(rw); // This call seems to leak memory no matter what.
				delete midi;
				break;
			}
		}
	}

	return audio;
}

bool midi::play_track(MidiTracks track, bool replay)
{
	auto midi = TrackToMidi(track);
	if (!midi || (!replay && active_track == track))
		return false;

	StopPlayback();

	if (!IsPlaying)
	{
		NextTrack = track;
		return false;
	}

	if (MixOpen && !StartTrack(midi))
	{
		active_track = MidiTracks::None;
		return false;
	}

	// On Windows, MIDI volume can only be set during playback.
	// And it changes application master volume for some reason.
	SetVolume(Volume);
	active_track = track;
	return true;
}

MusicHandle* midi::LoadFromRW(SDL_RWops* rw)
{
#ifdef MUSIC_TSF
	std::vector<uint8_t> data(static_cast<size_t>(std::max<Sint64>(SDL_RWsize(rw), 0)));
	auto read = SDL_RWread(rw, data.data(), 1, data.size());
	SDL_RWclose(rw);
	return read == data.size() ? tml_load_memory(data.data(), static_cast<int>(data.size())) : nullptr;
#else
	return Mix_LoadMUS_RW(rw, 1);
#endif
}

void midi::FreeTrack(MusicHandle* track)
{
#ifdef MUSIC_TSF
	tml_free(track);
#else
	Mix_FreeMusic(track);
#endif
}

bool midi::StartTrack(MusicHandle* track)
{
#ifdef MUSIC_TSF
	if (!Synth)
		return false;
	tsf_reset(Synth);
	SongStart = SongPos = track;
	SongTimeMs = 0;
	return true;
#else
	return Mix_PlayMusic(track, -1) == 0;
#endif
}

void midi::HaltTrack()
{
#ifdef MUSIC_TSF
	SongStart = SongPos = nullptr;
	if (Synth)
		tsf_note_off_all(Synth);
#else
	Mix_HaltMusic();
#endif
}

#ifdef MUSIC_TSF
// The web build is single-threaded: SDL calls this from the browser's audio callback on the main thread,
// so the song state needs no locking.
void midi::TsfCallback(void* udata, Uint8* stream, int len)
{
	constexpr int BlockSize = 64; // samples rendered between MIDI message checks
	auto out = reinterpret_cast<short*>(stream);
	for (int samples = len / (2 * sizeof(short)); samples > 0;)
	{
		auto block = std::min(samples, BlockSize);
		for (SongTimeMs += block * MsPerSample; SongPos && SongTimeMs >= SongPos->time;)
		{
			switch (SongPos->type)
			{
			case TML_PROGRAM_CHANGE:
				tsf_channel_set_presetnumber(Synth, SongPos->channel, SongPos->program, SongPos->channel == 9);
				break;
			case TML_NOTE_ON:
				tsf_channel_note_on(Synth, SongPos->channel, SongPos->key, SongPos->velocity / 127.0f);
				break;
			case TML_NOTE_OFF:
				tsf_channel_note_off(Synth, SongPos->channel, SongPos->key);
				break;
			case TML_PITCH_BEND:
				tsf_channel_set_pitchwheel(Synth, SongPos->channel, SongPos->pitch_bend);
				break;
			case TML_CONTROL_CHANGE:
				tsf_channel_midi_control(Synth, SongPos->channel, SongPos->control, SongPos->control_value);
				break;
			default:
				break;
			}

			// Loop forever, like Mix_PlayMusic(track, -1)
			SongPos = SongPos->next;
			if (!SongPos)
			{
				SongPos = SongStart;
				SongTimeMs = 0;
			}
		}

		tsf_render_short(Synth, out, block, 0);
		out += block * 2;
		samples -= block;
	}
}
#endif

MidiTracks midi::get_active_track()
{
	if (!IsPlaying)
		return NextTrack;
	else
		return active_track;
}

MusicHandle* midi::TrackToMidi(MidiTracks track)
{
	MusicHandle* midi;
	switch (track)
	{
	default:
	case MidiTracks::None:
		midi = nullptr;
		break;
	case MidiTracks::Track1:
		midi = track1;
		break;
	case MidiTracks::Track2:
		midi = track2;
		break;
	case MidiTracks::Track3:
		midi = track3;
		break;
	}
	return midi;
}

/// <summary>
/// SDL_mixed does not support MIDS. To support FT music, a conversion to MIDI is required.
/// </summary>
/// <param name="file">Path to .MDS file</param>
/// <returns>Vector that contains MIDI file</returns>
std::vector<uint8_t>* midi::MdsToMidi(std::string file)
{
	auto fileHandle = fopenu(file.c_str(), "rb");
	if (!fileHandle)
		return nullptr;

	fseek(fileHandle, 0, SEEK_END);
	auto fileSize = static_cast<uint32_t>(ftell(fileHandle));
	auto buffer = new uint8_t[fileSize];
	auto fileBuf = reinterpret_cast<riff_header*>(buffer);
	fseek(fileHandle, 0, SEEK_SET);
	fread(fileBuf, 1, fileSize, fileHandle);
	fclose(fileHandle);

	int returnCode = 0;
	std::vector<uint8_t>* midiOut = nullptr;
	do
	{
		if (fileSize < 12)
		{
			returnCode = 3;
			break;
		}
		if (fileBuf->Riff != FOURCC('R', 'I', 'F', 'F') ||
			fileBuf->Mids != FOURCC('M', 'I', 'D', 'S') ||
			fileBuf->Fmt != FOURCC('f', 'm', 't', ' '))
		{
			returnCode = 3;
			break;
		}
		if (fileBuf->FileSize > fileSize - 8)
		{
			returnCode = 3;
			break;
		}
		if (fileSize - 12 < 8)
		{
			returnCode = 3;
			break;
		}
		if (fileBuf->FmtSize < 12 || fileBuf->FmtSize > fileSize - 12)
		{
			returnCode = 3;
			break;
		}

		auto streamIdUsed = fileBuf->dwFlags == 0;
		auto dataChunk = reinterpret_cast<riff_data*>(reinterpret_cast<char*>(&fileBuf->dwTimeFormat) + fileBuf->
			FmtSize);
		if (dataChunk->Data != FOURCC('d', 'a', 't', 'a'))
		{
			returnCode = 3;
			break;
		}
		if (dataChunk->DataSize < 4)
		{
			returnCode = 3;
			break;
		}

		auto srcPtr = dataChunk->Blocks;
		std::vector<midi_event> midiEvents{};
		for (auto blockIndex = dataChunk->BlocksPerChunk; blockIndex; blockIndex--)
		{
			auto eventSizeInt = streamIdUsed ? 3 : 2;
			auto eventCount = srcPtr->CbBuffer / (4 * eventSizeInt);

			auto currentTicks = srcPtr->TkStart;
			auto srcPtr2 = reinterpret_cast<uint32_t*>(srcPtr->AData);
			for (auto i = 0u; i < eventCount; i++)
			{
				currentTicks += srcPtr2[0];
				auto event = streamIdUsed ? srcPtr2[2] : srcPtr2[1];
				midiEvents.push_back({currentTicks, event});
				srcPtr2 += eventSizeInt;
			}

			srcPtr = reinterpret_cast<riff_block*>(&srcPtr->AData[srcPtr->CbBuffer]);
		}

		// MIDS events can be out of order in the file
		std::sort(midiEvents.begin(), midiEvents.end(), [](const midi_event& lhs, const midi_event& rhs)
		{
			return lhs.iTicks < rhs.iTicks;
		});

		// MThd chunk
		std::vector<uint8_t>& midiBytes = *new std::vector<uint8_t>();
		midiOut = &midiBytes;
		midi_header header(SwapByteOrderShort(static_cast<uint16_t>(fileBuf->dwTimeFormat)));
		auto headerData = reinterpret_cast<const uint8_t*>(&header);
		midiBytes.insert(midiBytes.end(), headerData, headerData + sizeof header);

		// MTrk chunk
		midi_track track(7);
		auto trackData = reinterpret_cast<const uint8_t*>(&track);
		midiBytes.insert(midiBytes.end(), trackData, trackData + sizeof track);
		auto lengthPos = midiBytes.size() - 4;

		auto prevTime = 0u;
		for (const auto& event : midiEvents)
		{
			assertm(event.iTicks >= prevTime, "MIDS events: negative delta-time");
			uint32_t delta = event.iTicks - prevTime;
			prevTime = event.iTicks;

			// Delta time is in variable quantity, Big Endian
			uint32_t deltaVarLen;
			auto count = ToVariableLen(delta, deltaVarLen);
			auto deltaData = reinterpret_cast<const uint8_t*>(&deltaVarLen);
			midiBytes.insert(midiBytes.end(), deltaData, deltaData + count);

			switch (event.iEvent >> 24)
			{
			case 0:
				{
					// Type 0 - MIDI short message. 3 bytes: xx p1 p2 00, where xx - message, p* - parameters
					// Some of the messages have only one parameter
					auto msgMask = (event.iEvent) & 0xF0;
					auto shortMsg = (msgMask == 0xC0 || msgMask == 0xD0);
					auto eventData = reinterpret_cast<const uint8_t*>(&event.iEvent);
					midiBytes.insert(midiBytes.end(), eventData, eventData + (shortMsg ? 2 : 3));
					break;
				}
			case 1:
				{
					// Type 1 - tempo change, 3 bytes: xx xx xx 01
					// Meta message, set tempo, 3 bytes payload
					const uint8_t metaSetTempo[] = {0xFF, 0x51, 0x03};
					midiBytes.insert(midiBytes.end(), metaSetTempo, metaSetTempo + 3);

					auto eventBE = SwapByteOrderInt(event.iEvent);
					auto eventData = reinterpret_cast<const uint8_t*>(&eventBE) + 1;
					midiBytes.insert(midiBytes.end(), eventData, eventData + 3);
					break;
				}
			default:
				assertm(0, "MIDS events: uknown event");
				break;
			}
		}

		// Meta message, end of track, 0 bytes payload
		const uint8_t metaEndTrack[] = {0x00, 0xFF, 0x2f, 0x00};
		midiBytes.insert(midiBytes.end(), metaEndTrack, metaEndTrack + 4);

		// Set final MTrk size
		auto lengthBE = SwapByteOrderInt(static_cast<uint32_t>(midiBytes.size()) - sizeof header - sizeof track);
		auto lengthData = reinterpret_cast<const uint8_t*>(&lengthBE);
		std::copy_n(lengthData, 4, midiBytes.begin() + lengthPos);
	}
	while (false);

	delete[] buffer;
	if (returnCode && midiOut)
	{
		delete midiOut;
		midiOut = nullptr;
	}

	return midiOut;
}
