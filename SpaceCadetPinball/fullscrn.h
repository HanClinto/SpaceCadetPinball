#pragma once

struct screen_region
{
	SDL_Rect Src; // in vscreen
	SDL_Rect Dst; // in the window
};

struct resolution_info
{
	int16_t ScreenWidth;
	int16_t ScreenHeight;
	int16_t TableWidth;
	int16_t TableHeight;
	int16_t ResolutionMenuId;
};

class fullscrn
{
public:
	static int screen_mode;
	static int display_changed;
	static const resolution_info resolution_array[3];
	static float ScaleX;
	static float ScaleY;
	static int OffsetX;
	static int OffsetY;
	// Portrait layout (a window much taller than wide, such as a phone held upright): the board at full width,
	// the sidebar below it. Regions[0] is the board, the rest are pieces of the sidebar.
	static bool Portrait;
	static std::vector<screen_region> Regions;

	static void init();
	static void shutdown();
	static int set_screen_mode(int isFullscreen);
	static void activate(int flag);
	static int GetResolution();
	static void SetResolution(int value);
	static int GetMaxResolution();
	static void window_size_changed();
	static SDL_Rect GetScreenRectFromPinballRect(SDL_Rect rect);
	static float GetScreenToPinballRatio();
private :
	static int resolution;

	static void LayoutPortrait(int width, int height, int menuHeight);
	static int enableFullscreen();
	static int disableFullscreen();
};
