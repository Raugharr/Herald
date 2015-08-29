/*
 * File: Video.c
 * Author: David Brotz
 */

#include "Video.h"

#include "Gui.h"
#include "GuiLua.h"
#include "Point.h"
#include "MapRenderer.h"

#include "../World.h"
#include "../sys/LuaCore.h"
#include "../sys/Log.h"

#include <lua/lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static struct KeyMouseState g_KeyMouseState = {0, 0, 0, 0, 0, 0, {0, 0}, 0};
static struct Widget* g_FocusWidget = NULL;
static struct {
	struct Widget* Widget;
	SDL_Point Offset;
} g_DraggableWidget;

SDL_Window* g_Window = NULL;
SDL_Renderer* g_Renderer = NULL;
int g_VideoOk = 1;
SDL_Texture* g_WindowTexture = NULL;
int g_VideoTimer = 0;

int VideoInit(void) {
	Log(ELOG_INFO, "Setting up video.");
	++g_Log.Indents;
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
		goto error;
	if((g_Window = SDL_CreateWindow(SDL_CAPTION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_WIDTH, SDL_HEIGHT, SDL_WINDOW_SHOWN)) == NULL)
		goto error;
	if((g_Renderer = SDL_CreateRenderer(g_Window, -1, 0)) == NULL)
		goto error;
	if(TTF_Init() == -1)
		goto error;
	g_WindowTexture = SDL_CreateTexture(g_Renderer, SDL_GetWindowPixelFormat(g_Window), SDL_TEXTUREACCESS_STREAMING, SDL_WIDTH, SDL_HEIGHT);
	SDL_SetTextureBlendMode(g_WindowTexture, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
	if(InitGUILua(g_LuaState) == 0)
		goto error;
	--g_Log.Indents;
	return 1;
	error:
	g_VideoOk = 0;
	--g_Log.Indents;
	return 0;
}

void VideoQuit(void) {
	struct GUIFocus* _Focus = NULL;

	TTF_Quit();
	SDL_DestroyWindow(g_Window);
	SDL_Quit();
	if(g_GUIEvents != NULL)
		DestroyGUIEvents(g_GUIEvents);
	while(g_Focus != NULL) {
		_Focus = g_Focus;
		g_Focus = g_Focus->Prev;
		free(_Focus);
	}
	QuitGUILua(g_LuaState);
}

struct GUIFocus* ChangeFocus_Aux(struct GUIFocus* _Focus, int _Change, int _Pos) {
	const struct Container* _Parent = _Focus->Parent;
	struct GUIFocus* _Temp = NULL;
	int _LastIndex = 0;
	int _Ct = 0;

	change: /* Decrement the index until the index is invalid. */
	while(_Ct < _Change) {
		_Focus->Index += _Pos;
		if((_Focus->Index < 0 || _Focus->Index >= _Parent->ChildrenSz))
			break;
		if((_Focus->Index >= 0  && _Focus->Index < _Parent->ChildrenSz) &&
				_Parent->Children[_Focus->Index] != NULL &&_Parent->Children[_Focus->Index]->CanFocus != 0)
			++_Ct;
	}
	/* If the index is invalid focus the parent's next child
	 * if it exists else set index to the last widget. */
	if(_Focus->Index < 0 || _Focus->Index >= _Parent->ChildrenSz) {
		new_stack:
		if(_Parent->Parent != NULL) {
			_Temp = _Focus;

			_Focus = _Focus->Prev;
			_Parent = _Focus->Parent;
			//_Ct = 0;
			goto change;
		} else {
			loop_focus:
			if(_Pos < 0)
				_Focus->Index = _Parent->ChildrenSz - 1;
			else
				_Focus->Index = 0;
			if(_Parent->Children[_Focus->Index] != NULL && _Parent->Children[_Focus->Index]->CanFocus != 0)
				++_Ct;
		}
	}
	/* Index is now valid decrement the remaining indexes. */
	if(_Ct < _Change)
		goto change;
	/* Ensure the valid index we have is a valid widget. */
	_LastIndex = _Focus->Index;
	while((_Focus->Index >= 0 && _Focus->Index < _Parent->ChildrenSz) && (_Parent->Children[_Focus->Index] == NULL || (_Parent->Children[_Focus->Index]->CanFocus == 0)))
		_Focus->Index += _Pos;
	/* Index is invalid go back up and repair index. */
	if(_Focus->Index < 0 || _Focus->Index >= _Parent->ChildrenSz) {
		_Focus->Index = _LastIndex;
		goto new_stack;
	}
	_Focus->Id = _Parent->Children[_Focus->Index]->Id;
	if(_Temp != NULL) {
		_Temp->Parent->OnUnfocus((struct Widget*)_Temp->Parent);
		if(_Focus->Parent->Children[0]->Id == _Temp->Parent->Id || ((_Temp->Index > 0 && _Temp->Index < _Temp->Parent->ChildrenSz) && _Temp->Parent->Children[_Temp->Index] != NULL)){
			_Focus = _Temp;
			_Parent = _Focus->Parent;
			_Temp = NULL;
			goto loop_focus;
		} else
			free(_Temp);
	}
	return _Focus;
}

void Events(void) {
	SDL_Event _Event;
	struct Container* _Screen = NULL;
	struct Widget* _Widget = NULL;

	GUIMessageCheck(&g_GUIMessageList);
	KeyMouseStateClear(&g_KeyMouseState);
	SDL_GetMouseState(&g_KeyMouseState.MousePos.x, &g_KeyMouseState.MousePos.y);
	while(SDL_PollEvent(&_Event) != 0) {
		switch(_Event.type) {
		case SDL_KEYUP:
		case SDL_KEYDOWN:
			g_KeyMouseState.KeyboardState = _Event.key.state;
			g_KeyMouseState.KeyboardButton = _Event.key.keysym.sym;
			g_KeyMouseState.KeyboardMod = _Event.key.keysym.mod;
			break;
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			g_KeyMouseState.MouseButton = _Event.button.button;
			g_KeyMouseState.MouseState = _Event.button.state;
			g_KeyMouseState.MouseClicks = _Event.button.clicks;
			g_KeyMouseState.MousePos.x = _Event.button.x;
			g_KeyMouseState.MousePos.y = _Event.button.y;
			break;
		case SDL_MOUSEMOTION:
			g_KeyMouseState.MouseMove = 1;
			g_KeyMouseState.MousePos.x = _Event.motion.x;
			g_KeyMouseState.MousePos.y = _Event.motion.y;
			break;
		}
	}
	_Screen = GetScreen(g_LuaState);
	if(g_KeyMouseState.MouseMove != 0) {
		if(g_FocusWidget != NULL)
			g_FocusWidget->OnUnfocus(g_FocusWidget);
		g_FocusWidget = _Screen->OnFocus((struct Widget*)_Screen, &g_KeyMouseState.MousePos);
		if(g_DraggableWidget.Widget != NULL) {
			SDL_Point _Pos = {g_KeyMouseState.MousePos.x - g_DraggableWidget.Offset.x, g_KeyMouseState.MousePos.y - g_DraggableWidget.Offset.y};
			g_DraggableWidget.Widget->SetPosition(g_DraggableWidget.Widget, &_Pos);
		}
	}
	if(g_KeyMouseState.MouseState == SDL_PRESSED) {
		struct Widget* _Widget = _Screen->OnClick((struct Widget*)_Screen, &g_KeyMouseState.MousePos);

		if(_Widget == NULL || _Widget->IsDraggable == 0)
			return;
		g_DraggableWidget.Widget = _Widget;
		g_DraggableWidget.Offset.x = g_KeyMouseState.MousePos.x - _Widget->Rect.x;
		g_DraggableWidget.Offset.y = g_KeyMouseState.MousePos.y - _Widget->Rect.y;
	} else if(g_KeyMouseState.MouseState == SDL_RELEASED && (_Widget = _Screen->OnClick((struct Widget*)_Screen, &g_KeyMouseState.MousePos)) != NULL && _Widget != (struct Widget*)_Screen) {
		LuaGuiGetRef(g_LuaState);
		lua_rawgeti(g_LuaState, -1, _Widget->LuaOnClickFunc);
		LuaCallFunc(g_LuaState, 0, 0, 0);
		g_DraggableWidget.Widget = NULL;
	} else {
		if(g_GameWorld.IsPaused  == 0)
			GameWorldEvents(&g_KeyMouseState, &g_GameWorld);
	}
}

void Draw(void) {
	struct Container* _Screen = NULL;
	if(g_VideoOk == 0)
		return;
	_Screen = GetScreen(g_LuaState);
	SDL_RenderClear(g_Renderer);


	GameWorldDraw(&g_KeyMouseState, &g_GameWorld);
	LuaMenuThink(g_LuaState);
	/*TODO: After menu creation check if screen is NULL then throw an error and
	 *prevent the menu from being displayed instead of checking every draw call here.
	 */
	if(_Screen != NULL) {
		for(int i = 0; i < _Screen->ChildCt; ++i)
			_Screen->Children[i]->OnDraw(_Screen->Children[i]);
		SDL_SetRenderDrawColor(g_Renderer, 0x7F, 0x7F, 0x7F, SDL_ALPHA_OPAQUE);
		_Screen->OnDebug((struct Widget*)_Screen);
		SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	}
	SDL_RenderPresent(g_Renderer);
	if(SDL_GetTicks() <= g_VideoTimer + 16)
		SDL_Delay(SDL_GetTicks() - g_VideoTimer);
	g_VideoTimer = SDL_GetTicks();
}

struct Font* CreateFont(const char* _Name, int _Size) {
	struct Font* _Ret = malloc(sizeof(struct Font));

	if((_Ret->Font = TTF_OpenFont(_Name, _Size)) == NULL)
		Log(ELOG_ERROR, TTF_GetError());
	_Ret->Name = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy(_Ret->Name, _Name);
	_Ret->Size = _Size;
	_Ret->Prev = NULL;
	_Ret->Next = NULL;
	_Ret->RefCt = 0;
	if(g_GUIFonts != NULL) {
		g_GUIFonts->Prev = _Ret;
		_Ret->Next = g_GUIFonts;
	}
	g_GUIFonts = _Ret;
	return _Ret;
}

void DestroyFont(struct Font* _Font) {
	if(--_Font->RefCt > 0 || (_Font == g_GUIDefs.Font && _Font->RefCt != -1))
		return;
	if(_Font == g_GUIFonts) {
		g_GUIFonts = _Font->Next;
	} else {
		_Font->Prev->Next = _Font->Next;
		_Font->Next->Prev = _Font->Prev;
	}
	TTF_CloseFont(_Font->Font);
	free(_Font->Name);
	free(_Font);
}

void DestroyGUIEvents(struct GUIEvents* _Events) {
	free(_Events->Events);
	free(_Events);
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
	SDL_Surface* _Temp = SDL_ConvertSurface(_Surface, SDL_GetWindowSurface(g_Window)->format, 0);

	if(_Temp == NULL)
		Log(ELOG_ERROR, SDL_GetError());
	SDL_FreeSurface(_Surface);
	return _Temp;
}

SDL_Texture* SurfaceToTexture(SDL_Surface* _Surface) {
	SDL_Texture* _Texture = SDL_CreateTextureFromSurface(g_Renderer, _Surface);
	SDL_FreeSurface(_Surface);
	return _Texture;
}

void FocusableWidgetNull(void) {
	g_FocusWidget = NULL;
}

const struct Widget* GetFocusableWidget(void) {
	return g_FocusWidget;
}
