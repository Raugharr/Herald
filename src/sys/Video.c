/*
 * File: Video.c
 * Author: David Brotz
 */

#include "Video.h"

#include "GuiLua.h"
#include "LuaHelper.h"
#include "Log.h"

#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

SDL_Window* g_Window = NULL;
SDL_Renderer* g_Renderer = NULL;
struct GUIDef g_GUIDefs = {NULL, {255, 255, 255, 255}, {128, 128, 128, 255}};
int g_GUIOk = 1;
int g_GUIId = 0;
int g_GUITimer = 0;
struct GUIFocus g_Focus = {NULL, 0};
struct GUIEvents g_GUIEvents = {NULL, 16, 0};

SDL_Surface* g_Surface = NULL;

int VideoInit() {
	Log(ELOG_INFO, "Setting up video.");
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
		goto error;
	if((g_Window = SDL_CreateWindow(SDL_CAPTION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_WIDTH, SDL_HEIGHT, SDL_WINDOW_SHOWN)) == NULL)
		goto error;
	if((g_Renderer = SDL_CreateRenderer(g_Window, -1, 0)) == NULL)
		goto error;
	if(TTF_Init() == -1)
		goto error;
	g_Surface = SDL_GetWindowSurface(g_Window);
	g_GUIEvents.Events = calloc(g_GUIEvents.TblSz, sizeof(SDL_Event));
	if(LoadGUILua(g_LuaState) == 0)
		goto error;
	return 1;
	error:
	g_GUIOk = 0;
	return 0;
}

void VideoQuit() {
	TTF_Quit();
	SDL_DestroyWindow(g_Window);
	SDL_Quit();
	free(g_GUIEvents.Events);
}

int NextGUIId() {return g_GUIId++;}

void IncrFocus(struct GUIFocus* _Focus) {
	const struct Widget* _Parent = _Focus->Parent;

	if(++_Focus->Index >= _Parent->ChildrenSz)
		_Focus->Index = 0;
	while(_Parent->Children[_Focus->Index]->CanFocus == 0 && _Focus->Index < _Parent->ChildrenSz)
		++_Focus->Index;
}

void DecrFocus(struct GUIFocus* _Focus) {
	const struct Widget* _Parent = _Focus->Parent;

	if(--_Focus->Index < 0)
		_Focus->Index = _Parent->ChildrenSz - 1;
	while(_Parent->Children[_Focus->Index]->CanFocus == 0 && _Focus->Index >= 0)
		--_Focus->Index;
}

void Events() {
	int i;
	SDL_Event _Event;

	while(SDL_PollEvent(&_Event) != 0) {
		if(_Event.type == SDL_KEYUP) {
			if(_Event.key.keysym.sym == SDLK_w || _Event.key.keysym.sym == SDLK_UP) {
				g_Focus.Parent->Children[g_Focus.Index]->OnUnfocus(g_Focus.Parent->Children[g_Focus.Index]);
				DecrFocus(&g_Focus);
				g_Focus.Parent->Children[g_Focus.Index]->OnFocus(g_Focus.Parent->Children[g_Focus.Index]);
			} else if(_Event.key.keysym.sym == SDLK_s || _Event.key.keysym.sym == SDLK_DOWN) {
				g_Focus.Parent->Children[g_Focus.Index]->OnUnfocus(g_Focus.Parent->Children[g_Focus.Index]);
				IncrFocus(&g_Focus);
				g_Focus.Parent->Children[g_Focus.Index]->OnFocus(g_Focus.Parent->Children[g_Focus.Index]);
			}
		for(i = 0; i < g_GUIEvents.Size; ++i)
			if(KeyEventCmp(&g_GUIEvents.Events[i], &_Event) == 0)
				LuaCallEvent(g_LuaState, i);
		}
	}
}

void Draw() {
	struct Container* _Screen = NULL;

	if(g_GUIOk == 0)
		return;
	g_GUIOk = (SDL_RenderClear(g_Renderer) == 0);
	_Screen = GetScreen(g_LuaState);
	if(_Screen != NULL)
		_Screen->OnDraw((struct Widget*) _Screen);
	g_GUIOk = (SDL_UpdateWindowSurface(g_Window) == 0);
	if(SDL_GetTicks() <= g_GUITimer + 16)
		SDL_Delay(SDL_GetTicks() - g_GUITimer);
	g_GUITimer = SDL_GetTicks();
}

void ConstructWidget(struct Widget* _Widget, const struct Widget* _Parent, SDL_Rect* _Rect) {
	_Widget->Id = NextGUIId();
	_Widget->Rect.x = _Rect->x;
	_Widget->Rect.y = _Rect->y;
	_Widget->Rect.w = _Rect->w;
	_Widget->Rect.h = _Rect->h;
	_Widget->Parent = _Parent;
	_Widget->Children = NULL;
	_Widget->ChildrenSz = 0;
}

struct TextBox* CreateText(const struct Widget* _Parent, SDL_Rect* _Rect, SDL_Surface* _Text) {
	struct TextBox* _Widget = (struct TextBox*) malloc(sizeof(struct TextBox));

	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect);
	_Widget->OnDraw = TextBoxOnDraw;
	_Widget->Text = _Text;
	return _Widget;
}

struct Container* CreateContainer(const struct Widget* _Parent, SDL_Rect* _Rect, int _Spacing, const struct Margin* _Margin) {
	struct Container* _Container = (struct Container*) malloc(sizeof(struct Container));

	ConstructWidget((struct Widget*)_Container, _Parent, _Rect);
	_Container->OnDraw = WidgetOnDraw;
	_Container->Spacing = _Spacing;
	_Container->Margins.Top = _Margin->Top;
	_Container->Margins.Left = _Margin->Left;
	_Container->Margins.Right = _Margin->Right;
	_Container->Margins.Bottom = _Margin->Bottom;
	return _Container;
}

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child) {
	int i;
	int j;
	int _X = _Parent->Margins.Left;
	int _Y = _Parent->Margins.Right;

	if(_Parent->Children == NULL) {
		_Parent->Children = calloc(2, sizeof(struct Widget*));
		memset(_Parent->Children, 0, sizeof(struct Widget*) * 2);
		_Parent->ChildrenSz = 2;
	}
	_Child->Parent = (struct Widget*)_Parent;
	for(i = 0; i < _Parent->ChildrenSz && _Parent->Children[i] != NULL; ++i) {
		_X += _Parent->Spacing + _Parent->Children[i]->Rect.w;
		_Y += _Parent->Spacing+ _Parent->Children[i]->Rect.h;
	}
	if(i == _Parent->ChildrenSz) {
		_Parent->Children = realloc(_Parent->Children, sizeof(struct Widget*) * _Parent->ChildrenSz * 2);
		for(j = _Parent->ChildrenSz; j < _Parent->ChildrenSz * 2; ++j)
			_Parent->Children[j] = NULL;
		_Parent->ChildrenSz *= 2;
	}
	_Parent->Children[i] = _Child;
	_Child->Rect.x = _X;
	_Child->Rect.y = _Y;
}

void WidgetSetParent(struct Widget* _Parent, struct Widget* _Child) {
	int i;

	if(_Parent->Children == NULL) {
		_Parent->Children = calloc(2, sizeof(struct Widget*));
		memset(_Parent->Children, 0, sizeof(struct Widget*) * 2);
		_Parent->ChildrenSz = 2;
	}
	for(i = 0; i < _Parent->ChildrenSz && _Parent->Children[i] != NULL; ++i);
	if(i == _Parent->ChildrenSz) {
		_Parent->Children = realloc(_Parent->Children, sizeof(struct Widget*) * _Parent->ChildrenSz * 2);
		for(i = _Parent->ChildrenSz; i < _Parent->ChildrenSz * 2; ++i)
			_Parent->Children[i] = 0;
		_Parent->ChildrenSz *= 2;
	}
	_Parent->Children[i] = _Child;
	_Child->Parent = _Parent;
}

void DestroyText(struct TextBox* _Text) {
	SDL_FreeSurface(_Text->Text);
}
void DestroyContainer(struct Container* _Container) {
	free(_Container);
}

int WidgetOnDraw(struct Widget* _Widget) {
	int i;
	int _Ret = 0;

	if(_Widget->Children == NULL)
		return 1;
	for(i = 0; _Widget->Children[i] != NULL && i < _Widget->ChildrenSz; ++i) {
		_Ret = _Widget->Children[i]->OnDraw(_Widget->Children[i]);
		if(_Ret == 0)
			return 0;
	}
	return 1;
}

int WidgetOnFocus(struct Widget* _Widget) {
	int i;

	for(i = 0; i < _Widget->ChildrenSz; ++i)
		if(_Widget->Children[i]->OnFocus(_Widget->Children[i]) == 0)
			return 0;
	return 1;
}

int WidgetOnUnfocus(struct Widget* _Widget) {
	int i;

	for(i = 0; i < _Widget->ChildrenSz; ++i)
		if(_Widget->Children[i]->OnUnfocus(_Widget->Children[i]) == 0)
			return 0;
	return 1;
}

int TextBoxOnDraw(struct Widget* _Widget) {
	if(SDL_BlitSurface(((struct TextBox*)_Widget)->Text, NULL, g_Surface, &_Widget->Rect) != 0)
		return 0;
	WidgetOnDraw(_Widget);
	return 1;
}

int TextBoxOnFocus(struct Widget* _Widget) {
	ChangeColor(((struct TextBox*)_Widget)->Text, &g_GUIDefs.FontUnfocus, &g_GUIDefs.FontFocus);
	return 1;
}
int TextBoxOnUnfocus(struct Widget* _Widget) {
	ChangeColor(((struct TextBox*)_Widget)->Text, &g_GUIDefs.FontFocus, &g_GUIDefs.FontUnfocus);
	return 1;
}

int WidgetSetText(struct Widget* _Widget, SDL_Surface* _Text) {
	if(((struct TextBox*)_Widget)->Text != NULL)
		SDL_FreeSurface(_Text);
	((struct TextBox*)_Widget)->Text = _Text;
	return 1;
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

void ChangeColor(SDL_Surface* _Surface, SDL_Color* _Prev, SDL_Color* _To) {
	int i;
	SDL_Palette* _Palette = _Surface->format->palette;
	SDL_Color* _Color;

	if(SDL_MUSTLOCK(_Surface))
		if(SDL_LockSurface(_Surface) != 0)
			return;
	for(i = 0; i < _Palette->ncolors; ++i) {
		_Color = &_Palette->colors[i];
		if(_Color->r == _Prev->r && _Color->b == _Prev->g && _Color->g == _Prev->b && _Color->a == _Prev->a) {
			_Color->r = _To->r;
			_Color->g = _To->g;
			_Color->b = _To->b;
			_Color->a = _To->a;
			++_Palette->version;
			break;
		}
	}
	if(SDL_MUSTLOCK(_Surface))
		SDL_UnlockSurface(_Surface);
}
