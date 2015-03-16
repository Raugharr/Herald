/*
 * File: Video.c
 * Author: David Brotz
 */

#include "Video.h"

#include "Gui.h"
#include "GuiLua.h"
#include "LuaHelper.h"
#include "Log.h"

#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

SDL_Window* g_Window = NULL;
SDL_Renderer* g_Renderer = NULL;
int g_VideoOk = 1;
SDL_Surface* g_WindowSurface = NULL;
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
	g_WindowSurface = SDL_GetWindowSurface(g_Window);
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

void Events(void) {
	int i;
	SDL_Event _Event;
	struct Widget* _Callback = NULL;

	while(SDL_PollEvent(&_Event) != 0) {
		if(_Event.type == SDL_KEYUP) {
			struct Widget* _Widget = g_Focus->Parent->Children[g_Focus->Index];

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
			} else if(_Event.key.keysym.sym == SDLK_a || _Event.key.keysym.sym == SDLK_LEFT) {
				_Widget->OnUnfocus(_Widget);
				g_Focus = ChangeFocus(g_Focus, _Widget->Parent->HorzFocChange(g_Focus->Parent));
				g_Focus->Parent->Children[g_Focus->Index]->OnFocus(g_Focus->Parent->Children[g_Focus->Index]);
			} else if(_Event.key.keysym.sym == SDLK_d || _Event.key.keysym.sym == SDLK_RIGHT) {
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

	if(g_VideoOk == 0)
		return;
	//SDL_RenderClear(g_Renderer);
	_Screen = GetScreen(g_LuaState);
	SDL_FillRect(g_WindowSurface, NULL, (g_GUIDefs.Background.r << 16) | (g_GUIDefs.Background.g << 8) | (g_GUIDefs.Background.b));
	if(_Screen != NULL)
		_Screen->OnDraw((struct Widget*) _Screen);
	else
		g_VideoOk = 0;
	//SDL_RenderPresent(g_Renderer);
	g_VideoOk &= (SDL_UpdateWindowSurface(g_Window) == 0);
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
	SDL_Surface* _Temp = SDL_ConvertSurface(_Surface, g_WindowSurface->format, 0);

	if(_Temp == NULL)
		Log(ELOG_ERROR, SDL_GetError());
	SDL_FreeSurface(_Surface);
	return _Temp;
}

void ChangeColor(SDL_Surface* _Surface, SDL_Color* _Prev, SDL_Color* _To) {
	int i;
	int j;
	int _Width = 0;
	int _Height = 0;
	Uint32* _Pixel;
	SDL_Color* _Color;
	SDL_Palette* _Palette = _Surface->format->palette;
	SDL_PixelFormat* _Format = _Surface->format;

	if(SDL_MUSTLOCK(_Surface))
		if(SDL_LockSurface(_Surface) != 0)
			return;
	if(_Format->BytesPerPixel == 1)
		goto palette;
	else if(_Format->BytesPerPixel != 4)
		return;
	_Width = _Surface->w;
	_Height = _Surface->h;
	for(i = 0; i < _Width; ++i) {
		for(j = 0; j < _Height; ++j) {
			_Pixel = (Uint32*)((Uint8*)_Surface->pixels + j * _Surface->pitch + i * sizeof(*_Pixel));
			if((((*_Pixel & _Format->Rmask) >> _Format->Rshift) << _Format->Rloss) == _Prev->r &&
					(((*_Pixel & _Format->Gmask) >> _Format->Gshift) << _Format->Gloss) == _Prev->g &&
					(((*_Pixel & _Format->Bmask) >> _Format->Bshift) << _Format->Bloss) == _Prev->b &&
					(((*_Pixel & _Format->Amask) >> _Format->Ashift) << _Format->Aloss) == (((_Prev->a & _Format->Amask) >> _Format->Ashift) << _Format->Aloss)) {
				*_Pixel = (_To->r << _Format->Rshift) | (_To->g << _Format->Gshift) | (_To->b << _Format->Bshift) | ((_To->a & _Format->Amask) << _Format->Ashift);
			}
		}
	}
	goto end;
	palette:
	for(i = 0; i < _Palette->ncolors; ++i) {
		_Color = &_Palette->colors[i];
		if(_Color->r == _Prev->r && _Color->g == _Prev->g && _Color->b == _Prev->b && _Color->a == _Prev->a) {
			_Color->r = _To->r;
			_Color->g = _To->g;
			_Color->b = _To->b;
			_Color->a = _To->a;
			++_Palette->version;
			break;
		}
	}
	end:
	if(SDL_MUSTLOCK(_Surface))
		SDL_UnlockSurface(_Surface);
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

/*SDL_Surface* CreateLine(int _X1, int _Y1, int _X2, int _Y2) {
	int _DeltaX = _X2 - _X1;
	int _DeltaY = _Y2 - _Y1;
	int x;
	int y = 0;
	float _Error = 0.0f;
	float _DeltaError = abs(_DeltaY / _DeltaX);
	SDL_Surface* _Surface = SDL_CreateRGBSurface(0, abs(_X1 - _X2), abs(_Y1, _Y2), 8, 0, 0, 0, 0);

	for(x = _X1; x < _X2; ++x) {
		_Surface->pixels
		_Error += _DeltaError;
		if(_Error >= 0.5f) {
			++y;
			_Error -= 1.0f;
		}
	}
	return _Surface;
}*/
