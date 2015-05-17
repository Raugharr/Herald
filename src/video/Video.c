/*
 * File: Video.c
 * Author: David Brotz
 */

#include "Video.h"

#include "Gui.h"
#include "GuiLua.h"
#include "../sys/LuaHelper.h"
#include "../sys/Log.h"
#include "Point.h"
#include "MapRenderer.h"
#include "../World.h"

#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

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

	QuitGUILua(g_LuaState);
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

#include "Tile.h"

void Events(void) {
	int i;
	SDL_Event _Event;
	struct Widget* _Callback = NULL;

	GUIMessageCheck(&g_GUIMessageList);
	while(SDL_PollEvent(&_Event) != 0) {
		if(_Event.type == SDL_KEYUP) {
			struct Widget* _Widget = g_Focus->Parent->Children[g_Focus->Index];

			if(_Event.key.type == SDLK_a)
				g_GameWorld.MapRenderer->Screen.Center.X -= TILE_WIDTH;
			else if(_Event.key.keysym.sym == SDLK_d)
				g_GameWorld.MapRenderer->Screen.Center.X += TILE_WIDTH;
			else if(_Event.key.keysym.sym == SDLK_w)
				g_GameWorld.MapRenderer->Screen.Center.Y -= TILE_HEIGHT;
			else if(_Event.key.keysym.sym == SDLK_s)
				g_GameWorld.MapRenderer->Screen.Center.Y += TILE_HEIGHT;
			if(_Event.key.type == SDL_KEYUP)
				_Widget->OnKeyUp(_Widget, &_Event.key);
			if(_Event.key.keysym.sym == SDLK_UP) {
				_Widget->OnUnfocus(_Widget);
				g_Focus = ChangeFocus(g_Focus, -_Widget->Parent->VertFocChange);
				g_Focus->Parent->Children[g_Focus->Index]->OnFocus(g_Focus->Parent->Children[g_Focus->Index]);
			} else if(_Event.key.keysym.sym == SDLK_DOWN) {
				_Widget->OnUnfocus(_Widget);
				g_Focus = ChangeFocus(g_Focus, _Widget->Parent->VertFocChange);
				g_Focus->Parent->Children[g_Focus->Index]->OnFocus(g_Focus->Parent->Children[g_Focus->Index]);
			} else if(_Event.key.keysym.sym == SDLK_LEFT) {
				_Widget->OnUnfocus(_Widget);
				g_Focus = ChangeFocus(g_Focus, _Widget->Parent->HorzFocChange(g_Focus->Parent));
				g_Focus->Parent->Children[g_Focus->Index]->OnFocus(g_Focus->Parent->Children[g_Focus->Index]);
			} else if(_Event.key.keysym.sym == SDLK_RIGHT) {
				_Widget->OnUnfocus(_Widget);
				g_Focus = ChangeFocus(g_Focus, _Widget->Parent->HorzFocChange(g_Focus->Parent));
				g_Focus->Parent->Children[g_Focus->Index]->OnFocus(g_Focus->Parent->Children[g_Focus->Index]);
			}
			for(i = 0; i < g_GUIEvents->Size; ++i) {
				/* FIXME: If the Lua references are ever out of order because an event is deleted than this loop will fail. */
				_Callback = NULL;
				if(g_GUIEvents->Events[i].WidgetId == g_Focus->Id) {
					_Callback = g_Focus->Parent->Children[g_Focus->Index];
					goto event_check;
				}
				if(g_GUIEvents->Events[i].WidgetId == g_Focus->Parent->Id) {
					_Callback = (struct Widget*)g_Focus->Parent;
					goto event_check;
				}
				continue;
				event_check:
				if(KeyEventCmp(&g_GUIEvents->Events[i].Event, &_Event) == 0) {
					LuaCallEvent(g_LuaState, g_GUIEvents->Events[i].RefId, _Callback);
					if(g_GUIMenuChange != 0) {
						g_GUIMenuChange = 0;
						return;
					}
				}
			}
		}
	}
}

void Draw(void) {
	struct Container* _Screen = NULL;
	struct Point _Pos;
	SDL_GetMouseState(&_Pos.X, &_Pos.Y);
	struct Tile* _Tile = ScreenToTile(g_GameWorld.MapRenderer, &_Pos);
	if(g_VideoOk == 0)
		return;
	_Screen = GetScreen(g_LuaState);
	SDL_RenderClear(g_Renderer);
	//MapRender(g_Renderer, g_GameWorld.MapRenderer);
	if(_Tile != NULL) {
		SDL_Rect _Rect = {_Tile->ScreenPos.X, _Tile->ScreenPos.Y, TILE_WIDTH, TILE_HEIGHT};

		SDL_RenderCopy(g_Renderer, g_GameWorld.MapRenderer->Selector, NULL, &_Rect);
	}
	if(_Screen != NULL)
		_Screen->OnDraw((struct Widget*) _Screen);
	else
		g_VideoOk = 0;
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

int SDLEventCmp(const void* _One, const void* _Two) {
	if((((SDL_Event*)_One)->type == SDL_KEYDOWN || ((SDL_Event*)_Two)->type == SDL_KEYUP) &&
			(((SDL_Event*)_Two)->type == SDL_KEYDOWN || ((SDL_Event*)_Two)->type == SDL_KEYUP))
		return KeyEventCmp(_One, _Two);
	return 0;
}

int KeyEventCmp(const void* _One, const void* _Two) {
	if(((SDL_Event*)_One)->type != ((SDL_Event*)_Two)->type)
		return ((SDL_Event*)_One)->type - ((SDL_Event*)_Two)->type;
	if(((SDL_Event*)_One)->key.state != ((SDL_Event*)_Two)->key.state)
		return ((SDL_Event*)_One)->key.state - ((SDL_Event*)_Two)->key.state;
	if(((SDL_Event*)_One)->key.keysym.sym != ((SDL_Event*)_Two)->key.keysym.sym)
		return ((SDL_Event*)_One)->key.keysym.sym - ((SDL_Event*)_Two)->key.keysym.sym;
	if(((SDL_Event*)_One)->key.keysym.mod != ((SDL_Event*)_Two)->key.keysym.mod)
		return ((SDL_Event*)_One)->key.keysym.mod - ((SDL_Event*)_Two)->key.keysym.mod;
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

int FirstFocusable(const struct Container* _Parent) {
	int i;

	for(i = 0; i < _Parent->ChildrenSz; ++i) {
		if(_Parent->Children[i] == NULL)
			continue;
		if(_Parent->Children[i]->CanFocus != 0)
			return i;
	}
	return -1;
}

int NextFocusable(const struct Container* _Parent, int _Index, int _Pos) {
	if(_Index < 0)
		_Index = 0;
	else if(_Index >= _Parent->ChildrenSz)
		_Index = _Parent->ChildrenSz - 1;
	while((_Index >= 0 && _Index < _Parent->ChildrenSz) && (_Parent->Children[_Index] == NULL || (_Parent->Children[_Index]->CanFocus == 0)))
		_Index -= _Pos;
	return _Index;
}

int GetHorizontalCenter(const struct Container* _Parent, const struct Widget* _Widget) {
	return ((_Parent->Rect.x + _Parent ->Rect.w) / 2) - ((_Widget->Rect.x + _Widget ->Rect.w) / 2);
}
