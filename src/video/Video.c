/*
 * File: Video.c
 * Author: David Brotz
 */

#include "Video.h"

#include "Gui.h"
#include "GuiLua.h"
#include "MapRenderer.h"

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
	if(IMG_Init(IMG_INIT_PNG) == 0) {
		Log(ELOG_ERROR, IMG_GetError());
		goto error;
	}
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

void Draw(void) {
	struct Container* _Container = GUIZBot();
	if(g_VideoOk == 0)
		return;
	SDL_RenderClear(g_Renderer);

	GameWorldDraw(&g_GameWorld);
	LuaMenuThink(g_LuaState);
		while(_Container != NULL) {
			(_Container)->OnDraw((struct Widget*) _Container);
			_Container = _Container->Prev;
		}
		SDL_SetRenderDrawColor(g_Renderer, 0x7F, 0x7F, 0x7F, SDL_ALPHA_OPAQUE);
		_Container = GUIZBot();
		while(_Container != NULL) {
			_Container->OnDebug((struct Widget*)_Container);
			_Container = _Container->Prev;
		}
		SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderPresent(g_Renderer);
	if(SDL_GetTicks() <= g_VideoTimer + 16)
		SDL_Delay(SDL_GetTicks() - g_VideoTimer);
	g_VideoTimer = SDL_GetTicks();
}

int VideoEvents(const struct KeyMouseState* _State) {
	struct Widget* _Widget = NULL;
	struct Container* _Container = NULL;

	if(_State->MouseMove != 0) {
		if(g_FocusWidget != NULL)
			g_FocusWidget->OnUnfocus(g_FocusWidget);
		_Container = GUIZBot();
		while(_Container != NULL) {
			if((g_FocusWidget = _Container->OnFocus((struct Widget*)_Container, &_State->MousePos)) != NULL)
				break;
			_Container = _Container->Prev;
		}
		if(g_DraggableWidget.Widget != NULL) {
			SDL_Point _Pos = {_State->MousePos.x - g_DraggableWidget.Offset.x, _State->MousePos.y - g_DraggableWidget.Offset.y};
			g_DraggableWidget.Widget->SetPosition(g_DraggableWidget.Widget, &_Pos);
		}
	}
	if(_State->MouseState == SDL_PRESSED) {
		_Container = GUIZBot();
		while(_Container != NULL) {
			if((_Widget = _Container->OnDrag((struct Widget*)_Container, &_State->MousePos)) != NULL)
				break;
			_Container = _Container->Prev;
		}
		if(_Widget == NULL || _Widget->IsDraggable == 0)
			return 1;
		g_DraggableWidget.Widget = _Widget;
		g_DraggableWidget.Offset.x = _State->MousePos.x - _Widget->Rect.x;
		g_DraggableWidget.Offset.y = _State->MousePos.y - _Widget->Rect.y;
		return 1;
	} else if(_State->MouseState == SDL_RELEASED) {
		_Container = GUIZTop();
		//Go from top most container to bottom most looking for a container that returns a value indicating
		//that it can be clicked.
		while(_Container != NULL) {
			if((_Widget = _Container->OnClick((struct Widget*)_Container, &_State->MousePos)) != NULL)
				break;
			_Container = _Container->Next;
		}
		 if(_Widget == NULL) {
			 return 0;
		}
			GuiZToTop(_Container);
			if(_Widget->LuaOnClickFunc >= 0) {
				LuaGuiGetRef(g_LuaState);
				lua_rawgeti(g_LuaState, -1, _Widget->LuaOnClickFunc);
				LuaCallFunc(g_LuaState, 0, 0, 0);
			}
			g_DraggableWidget.Widget = NULL;
			return 1;
	}
	return 0;
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
	g_FocusWidget = NULL;
}

const struct Widget* GetFocusableWidget(void) {
	return g_FocusWidget;
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
