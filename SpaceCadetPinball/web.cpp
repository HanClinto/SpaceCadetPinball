// Functions the web page calls (emscripten_shell.html): sizing the canvas, touch and motion input,
// and the shared high score table. Not used by desktop builds.
#include "pch.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

#include "fullscrn.h"
#include "high_score.h"
#include "options.h"
#include "pb.h"
#include "winmain.h"

extern "C"
{
	// The canvas's size in device pixels. The page sets its CSS size; SDL draws at this resolution.
	// pixelRatio (device pixels per CSS pixel) keeps the menus and dialogs readable on high-density screens.
	EMSCRIPTEN_KEEPALIVE void web_resize(int width, int height, float pixelRatio)
	{
		if (!winmain::MainWindow || width <= 0 || height <= 0)
			return;
		winmain::UiPixelRatio = pixelRatio > 0 ? pixelRatio : 1;
		if (winmain::ImIO)
			winmain::ImIO->FontGlobalScale = options::Options.UIScale * winmain::UiPixelRatio;
		SDL_SetWindowSize(winmain::MainWindow, width, height);
		fullscrn::window_size_changed();
	}

	// Press or release a GameBindings action (flippers, plunger, nudges, new game, pause, ...),
	// whatever keys it is bound to
	EMSCRIPTEN_KEEPALIVE void web_input(int binding, int pressed)
	{
		if (binding < static_cast<int>(GameBindings::Min) || binding >= static_cast<int>(GameBindings::Max))
			return;
		pb::BindingInput(static_cast<GameBindings>(binding), pressed != 0);
	}

	// 0: playing, 1: game over, 2: demo; +4 while paused; +8 with Full Tilt's data (CADET.DAT)
	EMSCRIPTEN_KEEPALIVE int web_game_state()
	{
		int state = pb::game_mode == GameModes::GameOver ? 1 : 0;
		if (pb::IsDemo())
			state = 2;
		if (winmain::single_step)
			state |= 4;
		if (pb::FullTiltMode)
			state |= 8;
		return state;
	}

	// One row (0-4) of the high score table, from the page's scoreboard (Module.scoreboard).
	// The game uses it for its "High Score n" messages and to say a game made the table.
	EMSCRIPTEN_KEEPALIVE void web_set_high_score(int position, const char* name, int score)
	{
		high_score::set_external_entry(position, name, score);
	}

	// Write settings (and the local high score table) to the settings file. The desktop game does this when it
	// exits; a page never does, so the page calls this before saving the file to IndexedDB.
	EMSCRIPTEN_KEEPALIVE void web_save_settings()
	{
		if (!ImGui::GetCurrentContext() || !ImGui::GetIO().IniFilename)
			return;
		options::uninit();
		high_score::write();
		ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
	}

	EMSCRIPTEN_KEEPALIVE void web_show_high_scores()
	{
		winmain::pause(false);
		pb::high_scores();
	}

	// Show or hide the game's menu bar, for a page that has a menu of its own (phones: too small to tap)
	EMSCRIPTEN_KEEPALIVE void web_show_menu(int show)
	{
		winmain::PageHasMenu = true;
		if (options::Options.ShowMenu != (show != 0))
			options::toggle(Menu1::Show_Menu);
	}
}
#endif
