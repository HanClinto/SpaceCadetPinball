#pragma once

#ifdef MUSIC_TSF
#include "tml.h"
#include "tsf.h"
// A loaded song is its first MIDI message; TinySoundFont plays it from SDL_mixer's music hook
using MusicHandle = tml_message;
#else
using MusicHandle = Mix_Music;
#endif

constexpr uint32_t SwapByteOrderInt(uint32_t val)
{
	return (val >> 24) |
		((val << 8) & 0x00FF0000) |
		((val >> 8) & 0x0000FF00) |
		(val << 24);
}

constexpr uint16_t SwapByteOrderShort(uint16_t val)
{
	return static_cast<uint16_t>((val >> 8) | (val << 8));
}

#pragma pack(push)
#pragma pack(1)
struct riff_block
{
	uint32_t TkStart;
	uint32_t CbBuffer;
	char AData[4];
};

struct riff_data
{
	uint32_t Data;
	uint32_t DataSize;
	uint32_t BlocksPerChunk;
	riff_block Blocks[1];
};

struct riff_header
{
	uint32_t Riff;
	uint32_t FileSize;
	uint32_t Mids;
	uint32_t Fmt;
	uint32_t FmtSize;
	uint32_t dwTimeFormat;
	uint32_t cbMaxBuffer;
	uint32_t dwFlags;
	riff_data Data;
};

struct midi_event
{
	uint32_t iTicks;
	uint32_t iEvent;
};

struct midi_header
{
	explicit midi_header(uint16_t tickdiv)
		: tickdiv(tickdiv)
	{
	}

	const char MThd[4]{'M', 'T', 'h', 'd'};
	const uint32_t chunklen = SwapByteOrderInt(6);
	const int16_t format = SwapByteOrderShort(0);
	const uint16_t ntracks = SwapByteOrderShort(1);
	uint16_t tickdiv;
};

struct midi_track
{
	explicit midi_track(uint32_t chunklen)
		: chunklen(chunklen)
	{
	}

	const char MTrk[4]{'M', 'T', 'r', 'k'};
	uint32_t chunklen;
};

static_assert(sizeof(riff_block) == 0xC, "Wrong size of riff_block");
static_assert(sizeof(riff_data) == 0x18, "Wrong size of riff_data");
static_assert(sizeof(riff_header) == 0x38, "Wrong size of riff_header");
static_assert(sizeof(midi_event) == 8, "Wrong size of midi_event2");
static_assert(sizeof(midi_header) == 14, "Wrong size of midi_header");
static_assert(sizeof(midi_track) == 8, "Wrong size of midi_track");

#pragma pack(pop)

enum class MidiTracks
{
	None,
	Track1,
	Track2,
	Track3
};

class midi
{
public:
	static int music_init(bool mixOpen, int volume);
	static void music_shutdown();
	static void music_play();
	static void music_stop();
	static void SetVolume(int volume);
	static bool play_track(MidiTracks track, bool replay);
	static MidiTracks get_active_track();
private:
	static std::vector<MusicHandle*> LoadedTracks;
	static MusicHandle* track1, * track2, * track3;
	static MidiTracks active_track, NextTrack;
	static int Volume;
	static bool IsPlaying, MixOpen;

	static void StopPlayback();
	static MusicHandle* load_track(std::string fileName);
	static MusicHandle* load_track_sub(std::string fileName, bool isMidi);
	static MusicHandle* TrackToMidi(MidiTracks track);
	static std::vector<uint8_t>* MdsToMidi(std::string file);

	static MusicHandle* LoadFromRW(SDL_RWops* rw);
	static void FreeTrack(MusicHandle* track);
	static bool StartTrack(MusicHandle* track);
	static void HaltTrack();
#ifdef MUSIC_TSF
	static tsf* Synth;
	static MusicHandle *SongStart, *SongPos;
	static double SongTimeMs, MsPerSample;
	static void TsfCallback(void* udata, Uint8* stream, int len);
#endif
};
