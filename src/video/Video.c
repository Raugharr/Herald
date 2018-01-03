/*
 * File: Video.c
 * Author: David Brotz
 */

#include "Video.h"

#include "Gui.h"
#include "GuiLua.h"
#include "MapRenderer.h"
#include "TextRenderer.h"

#include "../World.h"
#include "../sys/LuaCore.h"
#include "../sys/Log.h"
#include "../sys/Event.h"

#include <lua/lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

static struct Widget* g_HoverWidget = NULL;
struct HoverWidget {
	struct Widget* Widget;
	uint32_t StartTime;
	bool Fired;
};

static struct {
	struct Widget* Widget;
	SDL_Point Offset;
} g_DraggableWidget;

SDL_Window* g_Window = NULL;
SDL_Renderer* g_Renderer = NULL;
int g_VideoOk = 1;
SDL_Texture* g_WindowTexture = NULL;
int g_VideoTimer = 0;
static struct Stack g_GovCols = {0};
static struct HoverWidget g_LHWidget = {0x0, 0, false};
SDL_Texture* g_VHex = NULL;

#define ColAlpha (0xFF)

static const struct LuaEnum g_LuaKeyCodes[] = {
	{"0", SDLK_0},
	{"1", SDLK_1},
	{"2", SDLK_2},
	{"3", SDLK_3},
	{"4", SDLK_4},
	{"5", SDLK_5},
	{"6", SDLK_6},
	{"7", SDLK_7},
	{"8", SDLK_8},
	{"9", SDLK_9},
	{"a", SDLK_a},
	{"b", SDLK_b},
	{"c", SDLK_c},
	{"d", SDLK_d},
	{"e", SDLK_e},
	{"f", SDLK_f},
	{"g", SDLK_g},
	{"h", SDLK_h},
	{"i", SDLK_i},
	{"j", SDLK_j},
	{"k", SDLK_k},
	{"l", SDLK_l},
	{"m", SDLK_m},
	{"n", SDLK_n},
	{"o", SDLK_o},
	{"p", SDLK_p},
	{"q", SDLK_q},
	{"r", SDLK_r},
	{"s", SDLK_s},
	{"t", SDLK_t},
	{"u", SDLK_u},
	{"v", SDLK_v},
	{"w", SDLK_w},
	{"x", SDLK_x},
	{"y", SDLK_y},
	{"z", SDLK_z},
	{"Return", SDLK_RETURN},
	{"(", SDLK_RIGHTPAREN},
	{")", SDLK_LEFTPAREN},
	{"`", SDLK_BACKQUOTE},
	{NULL, 0}
};

static const struct LuaEnum g_LuaModCodes[] = {
	{"None", KMOD_NONE},
	{"LShift", KMOD_LSHIFT},
	{"RShift", KMOD_RSHIFT},
	{"LCtrl", KMOD_LCTRL},
	{"RCtrl", KMOD_RCTRL},
	{"RAlt", KMOD_RALT},
	{"LAlt", KMOD_LALT},
	{"LGui", KMOD_LGUI},
	{"RGui", KMOD_RGUI},
	{"Num", KMOD_NUM},
	{"Caps", KMOD_CAPS},
	{"Mode", KMOD_MODE},
	{"Ctrl", KMOD_CTRL},
	{"Shift", KMOD_SHIFT},
	{"Alt", KMOD_ALT},
	{"Gui", KMOD_GUI},
	{NULL, 0}
};

static const uint8_t g_Hexagon[] = {
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\000\000\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\000\000\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000"
  "\000\000\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
};
//Image mapping of a 1bit 64x42 hexagon.
/*static const uint8_t g_Hexagon[] = {
	0x00, 0x00, 0x1C, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x7F, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0xC0, 0x00, 0x00,
	0x00, 0x07, 0xFF, 0xE0, 0x00, 0x00,
	0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00,
	0x00, 0x1F, 0xFF, 0xFE, 0x00, 0x00,
	0x00, 0x7F, 0xFF, 0xFF, 0x00, 0x00,
	0x01, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
	0x07, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
	0x1F, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
	0x3F, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
	0x7F, 0xFF, 0xFF, 0xFF, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
	0xF7, 0xFF, 0xFF, 0xFF, 0x7F, 0x00,
	0xF0, 0xFF, 0xFF, 0xFF, 0x1F, 0x00,
	0x80, 0xFF, 0xFF, 0xFF, 0x0F, 0x00,
	0x00, 0xFE, 0xFF, 0xFF, 0x05, 0x00,
	0x00, 0xFC, 0xFF, 0xFF, 0x01, 0x00,
	0x00, 0xF8, 0xFF, 0xFF, 0x00, 0x00,
	0x00, 0xF0, 0xFF, 0x3F, 0x00, 0x00,
	0x00, 0xC0, 0xFF, 0x07, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0x03, 0x00, 0x00,
	0x00, 0x00, 0xFE, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x30, 0x00, 0x00, 0x00
};*/

const struct LuaEnumReg g_LuaKeyboardEnums[] = {
	{"Keyboard", "Key", g_LuaKeyCodes},
	{"Keyboard", "Mod", g_LuaModCodes},
	{NULL, NULL}
};

int LuaKeyboardMod(lua_State* State) {
	int One = luaL_checkinteger(State, 1);
	int Two = luaL_checkinteger(State, 2);

	lua_pushboolean(State, One & Two);
	return 1;
}

int VideoInit(void) {
	//uint8_t ColAlpha = 0x40;
	static SDL_Color Colors[] = {
			{0x33, 0x33, 0x99, ColAlpha},
			{0x00, 0x87, 0xBD, ColAlpha},
			{0x00, 0x93, 0xAF, ColAlpha},
			{0x1F, 0x75, 0xFE, ColAlpha},
			{0x00, 0x00, 0xCD, ColAlpha},
			{0x00, 0x70, 0xBB, ColAlpha},
			{0x54, 0x5A, 0xA7, ColAlpha},
			{0x10, 0x34, 0xA6, ColAlpha},
			{0x3F, 0x00, 0xFF, ColAlpha},
			{0x19, 0x19, 0x70, ColAlpha},
			{0x4C, 0x51, 0x6D, ColAlpha},
		//RED
			{0xFF, 0xC0, 0xCB, ColAlpha},
			{0xF8, 0x83, 0x79, ColAlpha},
			{0xCD, 0x5C, 0x5C, ColAlpha},
			{0x96, 0x00, 0x18, ColAlpha},
			{0xE0, 0x11, 0x5F, ColAlpha},
			{0xB3, 0x1C, 0x1B, ColAlpha},
			{0xA4, 0x5A, 0x52, ColAlpha},
			{0x80, 0x00, 0x00, ColAlpha},
		//GREEN
			{0x25, 0x41, 0x17, ColAlpha},
			{0x30, 0x67, 0x54, ColAlpha},
			{0x34, 0x72, 0x35, ColAlpha},
			{0x6A, 0xA1, 0x21, ColAlpha},
	};
	SDL_Surface* Surface = NULL;

	Log(ELOG_INFO, "Setting up video.");
	++g_Log.Indents;

	for(int i = 0; i < sizeof(Colors) / sizeof(SDL_Color); ++i)
		StackPush(&g_GovCols, &Colors[i]);
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		Log(ELOG_ERROR, "%s", SDL_GetError());
		goto error;
	}
	if((g_Window = SDL_CreateWindow(SDL_CAPTION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_WIDTH, SDL_HEIGHT, SDL_WINDOW_SHOWN)) == NULL) {
		Log(ELOG_ERROR, "%s", SDL_GetError());
		goto error;
	}
	if((g_Renderer = SDL_CreateRenderer(g_Window, -1, 0)) == NULL) {
		Log(ELOG_ERROR, "%s", SDL_GetError());
		goto error;
	}
	if(TTF_Init() == -1)
		goto error;
	if(InitTextRenderer() == false) goto error;
	g_WindowTexture = SDL_CreateTexture(g_Renderer, SDL_GetWindowPixelFormat(g_Window), SDL_TEXTUREACCESS_STREAMING, SDL_WIDTH, SDL_HEIGHT);
	SDL_SetTextureBlendMode(g_WindowTexture, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
	if(InitGUILua(g_LuaState) == 0)
		goto error;
	--g_Log.Indents;
	if(IMG_Init(IMG_INIT_PNG) == 0) {
		Log(ELOG_ERROR, IMG_GetError());
		goto error;
	}
	lua_newtable(g_LuaState);
	//LuaAddEnum(g_LuaState, -1, g_LuaKeyCodes);
	RegisterLuaEnums(g_LuaState, g_LuaKeyboardEnums);
	lua_setglobal(g_LuaState, "KeyCode");
	lua_pushcfunction(g_LuaState, LuaKeyboardMod);
	lua_setglobal(g_LuaState, "KeyboardMod");

	/*Surface = SDL_CreateRGBSurface(0, TILE_WIDTH, TILE_HEIGHT, 1, 0x00, 0x00, 0x00, 0x00);
	for(int c = 0; c < TILE_HEIGHT; ++c) {
		for(int r = 0; r < 5; ++r) {
			uint8_t* t = &((uint8_t*)Surface->pixels)[Surface->pitch * c + r];

			for(int i = 0; i < 8; ++i) {
				*t = ((*t) | (g_Hexagon[c * 6 + r] & (1 << i)));
			}
		}
		uint8_t* t = &((uint8_t*)Surface->pixels)[Surface->pitch * c + 6];
		*t = ((*t) | (g_Hexagon[c * 6 + 6] & (1 << 0)));
		*t = ((*t) | (g_Hexagon[c * 6 + 6] & (1 << 1)));
	}*/
	Surface = SDL_CreateRGBSurfaceFrom((void*)g_Hexagon, TILE_WIDTH, TILE_HEIGHT, 16, 2 * TILE_WIDTH, 0x00, 0x00, 0x00, 0x00);
	//SDL_Color Black = {0, 0, 0, 0xFF};
	//SDL_Color White = {0xFF, 0xFF, 0xFF, 0xFF};
	//Surface->format->palette->colors[0] = Black;
	//Surface->format->palette->colors[1] = White; 
	SDL_SetColorKey(Surface, SDL_TRUE, 0);
	SDL_SaveBMP(Surface, "Hex.bmp");
	g_VHex = SurfaceToTexture(Surface);
	SDL_SetTextureAlphaMod(g_VHex, 0xA0);
	return 1;
	error:
	g_VideoOk = 0;
	--g_Log.Indents;
	return 0;
}

void VideoQuit(void) {
	struct GUIFocus* _Focus = NULL;

	SDL_DestroyTexture(g_VHex);
	QuitTextRenderer();
	TTF_Quit();
	SDL_DestroyWindow(g_Window);
	SDL_Quit();
	while(g_Focus != NULL) {
		_Focus = g_Focus;
		g_Focus = g_Focus->Prev;
		free(_Focus);
	}
	QuitGUILua(g_LuaState);
}


void Draw(void) {
	if(g_VideoOk == 0)
		return;
	SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(g_Renderer);

	GameWorldDraw(&g_GameWorld);
	//LuaMenuThink(g_LuaState);
	GuiMenuThink(g_LuaState);
	GuiDraw();
	SDL_SetRenderDrawColor(g_Renderer, 0x7F, 0x7F, 0x7F, SDL_ALPHA_OPAQUE);
	//GuiDrawDebug();
	SDL_RenderPresent(g_Renderer);


	if(SDL_GetTicks() <= g_VideoTimer + 16)
		SDL_Delay(SDL_GetTicks() - g_VideoTimer);
	g_VideoTimer = SDL_GetTicks();
}

int VideoEvents(const struct KeyMouseState* _State) {
	struct Widget* _Widget = NULL;
	struct Container* _Container = NULL;
	lua_State* _LuaState = g_LuaState;

	if(g_LHWidget.Widget == g_HoverWidget && g_LHWidget.Fired == false && SDL_TICKS_PASSED(SDL_GetTicks(), g_LHWidget.StartTime + 1000) != 0) {
		g_LHWidget.Fired = true;
		if(g_LHWidget.Widget != NULL)
			LuaGuiCallFunc(_LuaState, g_LHWidget.Widget, GUIL_ONHOVER, 0);
	}
	if(_State->MouseMove != 0) {
		if(g_HoverWidget != NULL)
			g_HoverWidget->OnUnfocus(g_HoverWidget);
		g_HoverWidget = GuiFind(offsetof(struct Widget, OnFocus), &_State->MousePos);	
		if(g_LHWidget.Widget != g_HoverWidget) {
			if(g_LHWidget.Fired == true && g_LHWidget.Widget != NULL) {
				LuaGuiCallFunc(_LuaState, g_LHWidget.Widget, GUIL_ONHOVERLOSS, 0);
				g_LHWidget.Fired = false;	
			}
			g_LHWidget.Widget = g_HoverWidget;
			g_LHWidget.StartTime = SDL_GetTicks();
		} 
		if(g_DraggableWidget.Widget != NULL) {
			SDL_Point _Pos = {_State->MousePos.x - g_DraggableWidget.Offset.x, _State->MousePos.y - g_DraggableWidget.Offset.y};
			g_DraggableWidget.Widget->SetPosition(g_DraggableWidget.Widget, &_Pos);
		}
	}
	if(_State->MouseState == SDL_PRESSED) {
		_Widget = GuiFind(offsetof(struct Widget, OnDrag), &_State->MousePos);
		if(_Widget == NULL || _Widget->IsDraggable == 0)
			return 1;
		g_DraggableWidget.Widget = _Widget;
		g_DraggableWidget.Offset.x = _State->MousePos.x - _Widget->Rect.x;
		g_DraggableWidget.Offset.y = _State->MousePos.y - _Widget->Rect.y;
		return 1;
	} else if(_State->MouseState == SDL_RELEASED) {
		g_HoverWidget = GuiFind(offsetof(struct Widget, OnClick), &_State->MousePos);
		if(g_HoverWidget == NULL || g_HoverWidget->Clickable == 0) return 0;
			if(g_HoverWidget->Parent == NULL) {
				_Container = (struct Container*) g_HoverWidget;
			} else {
				_Container = g_HoverWidget->Parent;
				while(_Container->Widget.Parent != NULL)
					_Container = _Container->Widget.Parent;
			}
			GuiZToTop(_Container);
			LuaGuiCallFunc(_LuaState, g_HoverWidget, GUIL_ONCLICK, 0); 
			g_DraggableWidget.Widget = NULL;
			return 1;
	}
	struct Widget* ClickWidget = (g_HoverWidget != NULL) ? (g_HoverWidget) : (GuiGetBack());
	union UWidgetOnKey KeyEvent;
	char c = '\0';
	if(_State->KeyboardButton != 0 && _State->KeyboardState == SDL_RELEASED) {
		KeyEvent.Type = WEVENT_KEY;
		KeyEvent.Key.Key = _State->KeyboardButton;
		KeyEvent.Key.Mod = _State->KeyboardMod;
		c = _State->KeyboardButton;
		lua_pushinteger(_LuaState, c);
		lua_pushinteger(_LuaState, _State->KeyboardMod);
		LuaGuiCallFunc(_LuaState, ClickWidget, GUIL_ONKEY, 2);
	} else if(_State->TextBuff[0] != '\0') {
		KeyEvent.Type = WEVENT_TEXT;
		KeyEvent.Text.Text = _State->TextBuff;
		c = _State->TextBuff[0];
	} else {
		goto end;
	}
	ClickWidget->OnKey(ClickWidget, &KeyEvent);
	/*if(_State->KeyboardButton != 0 && _State->KeyboardState == SDL_RELEASED) {
		struct Widget* ClickWidget = (g_HoverWidget != NULL) ? (g_HoverWidget) : (GuiGetBack());

		lua_pushinteger(_LuaState, _State->KeyboardButton);
		lua_pushinteger(_LuaState, _State->KeyboardMod);
		LuaGuiCallFunc(_LuaState, ClickWidget, GUIL_ONKEY, 2);
		ClickWidget->OnKey(ClickWidget, ((_State->TextBuff[0] == '\0') ? (_State->KeyboardButton) : (_State->TextBuff[0])), _State->KeyboardMod);
	}*/
	end:
	return 0;
}

void DestroyFocus(struct GUIFocus* _Focus) {
	struct GUIFocus* _Prev = NULL;

	if(_Focus == NULL)
		return;
	_Prev = _Focus->Prev;
	free(_Focus);
	DestroyFocus(_Prev);
}

int KeyEventCmp(const struct KeyMouseState* _One, const struct KeyMouseState* _Two) {
	if(_One->KeyboardButton != _Two->KeyboardButton)
		return _One->KeyboardButton - _Two->KeyboardButton;
	if(_One->KeyboardMod != _Two->KeyboardMod)
		return _One->KeyboardMod != _Two->KeyboardMod;
	if(_One->KeyboardState != _Two->KeyboardState)
		return _One->KeyboardState - _Two->KeyboardState;
	if(_One->MouseButton != _Two->MouseButton)
		return _One->MouseButton - _Two->MouseButton;
	if(_One->MouseClicks != _Two->MouseClicks)
		return _One->MouseClicks - _Two->MouseClicks;
	if((_One->MousePos.x != _Two->MousePos.x) || (_One->MousePos.y != _Two->MousePos.y))
		return _One->MousePos.x - _Two->MousePos.x;
	if(_One->MouseState != _Two->MouseState)
		return _One->MouseState - _Two->MouseState;
	return 0;
}

SDL_Surface* ConvertSurface(SDL_Surface* _Surface) {
	SDL_Surface* _Window = SDL_GetWindowSurface(g_Window);
	SDL_Surface* _Temp = NULL;

	if(_Surface == NULL) {
		Log(ELOG_ERROR, "ConvertSurface: Initial surface is NULL.");
		return NULL;
	}
	if(_Window == NULL) {
		Log(ELOG_ERROR, SDL_GetError());
		return NULL;
	}
	_Temp = SDL_ConvertSurface(_Surface, _Window->format, 0);
	if(_Temp == NULL)
		Log(ELOG_ERROR, SDL_GetError());
	SDL_FreeSurface(_Surface);
	return _Temp;
}

SDL_Texture* SurfaceToTexture(SDL_Surface* _Surface) {
	SDL_Texture* _Texture = SDL_CreateTextureFromSurface(g_Renderer, _Surface);

	SDL_FreeSurface(_Surface);
	if(_Texture == NULL)
		Log(ELOG_ERROR, "Cannot convert SDL surface: %s", SDL_GetError());
	return _Texture;
}

void FocusableWidgetNull(void) {
	g_HoverWidget = NULL;
	g_LHWidget.Widget = NULL;
}

const struct Widget* GetFocusableWidget(void) {
	return g_HoverWidget;
}

void* DownscaleImage(void* _Image, int _Width, int _Height, int _ScaleArea) {
	int _NewWidth = (_Width / _ScaleArea);
	int _NewSize = _NewWidth * (_Height / _ScaleArea);
	int x = 0;
	int y = 0;
	int i = 0;
	int _Ct = 0;
	int _Avg = 0;
	int _AvgCt = _ScaleArea * _ScaleArea;
	void* _NewImg = calloc(sizeof(int), _NewSize);

	for(i = 0; i < _NewSize; ++i) {
		for(x = 0; x < _ScaleArea; ++x)
			for(y = 0; y < _ScaleArea; ++y)
				_Avg += ((int*)_Image)[y * _ScaleArea + (_Ct * _ScaleArea + x)];
		++_Ct;
		if(_Ct > _NewWidth)
			_Ct = 0;
		((int*)_NewImg)[i] = _Avg / _AvgCt;
		_Avg = 0;
	}
	return _NewImg;
}

void ZoneColorDefault(SDL_Color* Color) {
	Color->r = 0;
	Color->b = 0;
	Color->g = 0;
	Color->a = 0x40;
}

void NewZoneColor(SDL_Color* Color) {
	SDL_Color* c = StackPop(&g_GovCols);

	*Color = *c;
}

void PutPixel(SDL_Surface* s, int x, int y) {
	uint8_t* dst = NULL;

	dst = (Uint8*) s->pixels + y * s->pitch + x * sizeof(Uint8);
	*dst = 1;
}

void LineFrom(SDL_Surface* s, int x0, int y0, int x1) {
	for(int x = x0; x < x1; ++x) {
		PutPixel(s, x, y0);
	}

}

SDL_Surface* CreateRoundRect(SDL_Rect Rect, int r) {
    int x = r -1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (r << 1);
	SDL_Surface* s = SDL_CreateRGBSurface(0, (Rect.w + r) * 2 - 1, (Rect.h + r) * 2 - 1, 8, 0, 0, 0, 0);
	SDL_Palette* Palette = s->format->palette;

	Palette->colors[0].r = 0;
	Palette->colors[0].b = 0;
	Palette->colors[0].g = 0;
	Palette->colors[1].r = 255;
	Palette->colors[1].g = 255;
	Palette->colors[1].b = 255;
	Rect.x = s->w / 2;
	Rect.y = s->h / 2;

    while (x >= y)
    {
    	LineFrom(s, Rect.x - y - Rect.w, Rect.y + x + Rect.h, Rect.x + y + Rect.w + 1);
        LineFrom(s, Rect.x - x - Rect.w, Rect.y + y + Rect.h, Rect.x + x + Rect.w + 1);
        LineFrom(s, Rect.x - x - Rect.w, Rect.y - y - Rect.h, Rect.x + x + Rect.w + 1);
        LineFrom(s, Rect.x - y - Rect.w, Rect.y - x - Rect.h, Rect.x + y + Rect.w + 1);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-r << 1) + dx;
        }
    }
	for(int i = 0; i < Rect.h; ++i) {
		LineFrom(s, Rect.x - Rect.w - r + 1, Rect.y - i, Rect.x + Rect.w + r - 1 + 1);
		LineFrom(s, Rect.x - Rect.w - r + 1, Rect.y + i, Rect.x + Rect.w + r - 1 + 1);
	}
	return s;
}

SDL_Surface* CreateRoundRectOut(SDL_Rect Rect, int r) {
    int x = r -1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (r << 1);
	SDL_Surface* s = SDL_CreateRGBSurface(0, (Rect.w + r) * 2 - 1, (Rect.h + r) * 2 - 1, 8, 0, 0, 0, 0);
	SDL_Palette* Palette = s->format->palette;

	Palette->colors[0].r = 255;
	Palette->colors[0].g = 255;
	Palette->colors[0].b = 255;
	Palette->colors[1].r = 0;
	Palette->colors[1].b = 0;
	Palette->colors[1].g = 0;
	Rect.x = s->w / 2;
	Rect.y = s->h / 2;

    while (x >= y)
    {
        PutPixel(s, Rect.x + x + Rect.w, Rect.y + y + Rect.h);
		PutPixel(s, Rect.x + y + Rect.w, Rect.y + x + Rect.h);
        PutPixel(s, Rect.x - y - Rect.w, Rect.y + x + Rect.h);
        PutPixel(s, Rect.x - x - Rect.w, Rect.y + y + Rect.h);
        PutPixel(s, Rect.x - x - Rect.w, Rect.y - y - Rect.h);
        PutPixel(s, Rect.x - y - Rect.w, Rect.y - x - Rect.h);
        PutPixel(s, Rect.x + y + Rect.w, Rect.y - x - Rect.h);
        PutPixel(s, Rect.x + x + Rect.w, Rect.y - y - Rect.h);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-r << 1) + dx;
        }
    }
	for(int i = 0; i < Rect.h; ++i) {
		PutPixel(s, Rect.x + Rect.w + r - 1, Rect.y + i);
		PutPixel(s, Rect.x + Rect.w + r - 1, Rect.y - i);
		PutPixel(s, Rect.x - Rect.w - r + 1, Rect.y + i);
		PutPixel(s, Rect.x - Rect.w - r + 1, Rect.y - i);
	}
	for(int i = 0; i < Rect.w; ++i) {
		PutPixel(s, Rect.x - i, Rect.y - Rect.h - r + 1);
		PutPixel(s, Rect.x + i, Rect.y - Rect.h - r + 1);
		PutPixel(s, Rect.x - i, Rect.y + Rect.h + r - 1);
		PutPixel(s, Rect.x + i, Rect.y + Rect.h + r - 1);
	}
	return s;
}
