#include "pch.h"
#include "fullscrn.h"


#include "options.h"
#include "pb.h"
#include "render.h"
#include "TPinballTable.h"
#include "winmain.h"


int fullscrn::screen_mode;
int fullscrn::display_changed;

int fullscrn::resolution = 0;
const resolution_info fullscrn::resolution_array[3] =
{
	{640, 480, 600, 416, 501},
	{800, 600, 752, 520, 502},
	{1024, 768, 960, 666, 503},
};
float fullscrn::ScaleX = 1;
float fullscrn::ScaleY = 1;
bool fullscrn::Portrait = false;
std::vector<screen_region> fullscrn::Regions{};
int fullscrn::OffsetX = 0;
int fullscrn::OffsetY = 0;

void fullscrn::init()
{
	window_size_changed();
}

void fullscrn::shutdown()
{
	if (display_changed)
		set_screen_mode(0);
}

int fullscrn::set_screen_mode(int isFullscreen)
{
	int result = isFullscreen;
	if (isFullscreen == screen_mode)
		return result;
#ifndef __EMSCRIPTEN__
	screen_mode = isFullscreen;
	if (isFullscreen)
	{
		enableFullscreen();
		result = 1;
	}
	else
	{
		disableFullscreen();
		result = 1;
	}
#endif
	return result;
}

int fullscrn::enableFullscreen()
{
	if (!display_changed)
	{
		if (SDL_SetWindowFullscreen(winmain::MainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0)
		{
			display_changed = 1;
			return 1;
		}
	}
	return 0;
}

int fullscrn::disableFullscreen()
{
	if (display_changed)
	{
		if (SDL_SetWindowFullscreen(winmain::MainWindow, 0) == 0)
			display_changed = 0;
	}

	return 0;
}

void fullscrn::activate(int flag)
{
	if (screen_mode)
	{
		if (!flag)
		{
			set_screen_mode(0);
		}
	}
}

int fullscrn::GetResolution()
{
	return resolution;
}

void fullscrn::SetResolution(int value)
{
	if (!pb::FullTiltMode || pb::FullTiltDemoMode)
		value = 0;
	assertm(value >= 0 && value <= 2, "Resolution value out of bounds");
	resolution = value;
}

int fullscrn::GetMaxResolution()
{
	return pb::FullTiltMode && !pb::FullTiltDemoMode ? 2 : 0;
}

void fullscrn::window_size_changed()
{
	int width, height;
	SDL_GetRendererOutputSize(winmain::Renderer, &width, &height);
	int menuHeight = options::Options.ShowMenu ? winmain::MainMenuHeight : 0;
	height -= menuHeight;

	Portrait = height > width * 1.2f && render::vscreen && pb::MainTable;
	if (Portrait)
	{
		LayoutPortrait(width, height, menuHeight);
		return;
	}

	auto res = &resolution_array[resolution];
	ScaleX = static_cast<float>(width) / res->TableWidth;
	ScaleY = static_cast<float>(height) / res->TableHeight;
	OffsetX = OffsetY = 0;
	auto offset2X = 0, offset2Y = 0;

	if (options::Options.IntegerScaling)
	{
		ScaleX = ScaleX < 1 ? ScaleX : std::floor(ScaleX);
		ScaleY = ScaleY < 1 ? ScaleY : std::floor(ScaleY);
	}

	if (options::Options.UniformScaling)
	{
		ScaleY = ScaleX = std::min(ScaleX, ScaleY);
	}

	offset2X = static_cast<int>(floor(width - res->TableWidth * ScaleX));
	offset2Y = static_cast<int>(floor(height - res->TableHeight * ScaleY));
	OffsetX = offset2X / 2;
	OffsetY = offset2Y / 2;

	render::DestinationRect = SDL_Rect
	{
		OffsetX, OffsetY + menuHeight,
		width - offset2X, height - offset2Y
	};
}

void fullscrn::LayoutPortrait(int width, int height, int menuHeight)
{
	auto vW = render::vscreen->Width, vH = render::vscreen->Height;
	auto boardW = pb::MainTable->Width, sideW = vW - boardW;

	// The sidebar is a tall logo panel over the score panels. Side by side they fit under the board.
	// Where the logo panel ends, measured at 3DPB's 416 and FT's 666 lines (the same at FT's other sizes).
	auto split = pb::FullTiltMode ? vH * 324 / 666 : vH * 184 / 416;
	Regions.clear();
	Regions.push_back({{0, 0, boardW, vH}, {}});
	Regions.push_back({{boardW, 0, sideW, split}, {}});
	Regions.push_back({{boardW, split, sideW, vH - split}, {}});

	int rowW = 0, rowH = 0;
	for (auto i = 1u; i < Regions.size(); i++)
	{
		rowW += Regions[i].Src.w;
		rowH = std::max(rowH, Regions[i].Src.h);
	}

	// The board and the row under it each fill the width, shrunk together if that's too tall
	auto boardScale = static_cast<float>(width) / boardW, rowScale = static_cast<float>(width) / rowW;
	auto fit = std::min(1.0f, height / (vH * boardScale + rowH * rowScale));
	boardScale *= fit;
	rowScale *= fit;
	if (options::Options.IntegerScaling && boardScale > 1)
	{
		rowScale *= std::floor(boardScale) / boardScale;
		boardScale = std::floor(boardScale);
	}
	ScaleX = ScaleY = boardScale;

	auto scaled = [](int value, float scale) { return static_cast<int>(std::round(value * scale)); };
	auto top = menuHeight + (height - scaled(vH, boardScale) - scaled(rowH, rowScale)) / 2;
	auto& board = Regions[0];
	board.Dst = {(width - scaled(boardW, boardScale)) / 2, top, scaled(boardW, boardScale), scaled(vH, boardScale)};
	auto x = (width - scaled(rowW, rowScale)) / 2;
	for (auto i = 1u; i < Regions.size(); i++)
	{
		auto& src = Regions[i].Src;
		Regions[i].Dst = {x, top + board.Dst.h, scaled(src.w, rowScale), scaled(src.h, rowScale)};
		x += Regions[i].Dst.w;
	}

	OffsetX = board.Dst.x;
	OffsetY = board.Dst.y - menuHeight;
	// The whole vscreen at the board's position and scale; the debug overlay uses it, only its board part shows
	render::DestinationRect = SDL_Rect{board.Dst.x, board.Dst.y, scaled(vW, boardScale), scaled(vH, boardScale)};
}

SDL_Rect fullscrn::GetScreenRectFromPinballRect(SDL_Rect rect)
{
	SDL_Rect converted_rect;

	if (Portrait)
	{
		for (const auto& region : Regions)
		{
			const auto& src = region.Src;
			if (rect.x >= src.x && rect.x < src.x + src.w && rect.y >= src.y && rect.y < src.y + src.h)
			{
				auto scale = static_cast<float>(region.Dst.w) / src.w;
				converted_rect.x = region.Dst.x + static_cast<int>((rect.x - src.x) * scale);
				converted_rect.y = region.Dst.y + static_cast<int>((rect.y - src.y) * scale);
				converted_rect.w = static_cast<int>(rect.w * scale);
				converted_rect.h = static_cast<int>(rect.h * scale);
				return converted_rect;
			}
		}
	}

	converted_rect.x = rect.x * render::DestinationRect.w / render::vscreen->Width + render::DestinationRect.x;
	converted_rect.y = rect.y * render::DestinationRect.h / render::vscreen->Height + render::DestinationRect.y;

	converted_rect.w = rect.w * render::DestinationRect.w / render::vscreen->Width;
	converted_rect.h = rect.h * render::DestinationRect.h / render::vscreen->Height;

	return converted_rect;
}

float fullscrn::GetScreenToPinballRatio()
{
	return (float) render::DestinationRect.w / render::vscreen->Width;
}