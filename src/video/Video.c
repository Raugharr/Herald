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
static struct HoverWidget g_LHWidget = {0x0, 0, false};

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
	Log(ELOG_INFO, "Setting up video.");
	++g_Log.Indents;
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
	return 1;
	error:
	g_VideoOk = 0;
	--g_Log.Indents;
	return 0;
}

void VideoQuit(void) {
	struct GUIFocus* _Focus = NULL;

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
	SDL_RenderClear(g_Renderer);

	GameWorldDraw(&g_GameWorld);
	LuaMenuThink(g_LuaState);
	GuiDraw();
	SDL_SetRenderDrawColor(g_Renderer, 0x7F, 0x7F, 0x7F, SDL_ALPHA_OPAQUE);
	//GuiDrawDebug();
	SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
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

void NewZoneColor(SDL_Color* _Color) {
	static SDL_Color _Colors[] = {
			{0xFF, 0xFF, 0xFF, 0x40},
			{0xFF, 0, 0, 0x40},
			{0, 0xFF, 0, 0x40},
			{0, 0, 0xFF, 0x40}
	};
	static int _Index = 0;

	if(_Index > 3)
		_Index = 0;
	*_Color = _Colors[_Index++];
}
