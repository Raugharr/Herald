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
int g_GUIMenuChange = 0;
int g_GUIId = 0;
int g_GUITimer = 0;
struct GUIFocus* g_Focus = NULL;
struct GUIEvents* g_GUIEvents = NULL;
SDL_Surface* g_Surface = NULL;
struct Font* g_GUIFonts = NULL;
struct Stack g_GUIStack = {NULL, 0};

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
	g_Surface = SDL_GetWindowSurface(g_Window);
	if(InitGUILua(g_LuaState) == 0)
		goto error;
	--g_Log.Indents;
	return 1;
	error:
	g_GUIOk = 0;
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

int NextGUIId(void) {return g_GUIId++;}

struct GUIFocus* ChangeFocus_Aux(struct GUIFocus* _Focus, int _Change, int _Pos) {
	const struct Container* _Parent = _Focus->Parent;
	struct GUIFocus* _Temp = NULL;
	int _LastIndex = _Focus->Index;

	change: /* Decrement the index until the index is invalid. */
	while(_Change > 0) {
		_Focus->Index += _Pos;
		if((_Focus->Index < 0 || _Focus->Index >= _Parent->ChildrenSz))
			break;
		if((_Focus->Index >= 0  && _Focus->Index < _Parent->ChildrenSz) &&
				_Parent->Children[_Focus->Index] != NULL &&_Parent->Children[_Focus->Index]->CanFocus != 0)
			--_Change;
	}
	/* If the index is invalid focus the parent's next child
	 * if it exists else set index to the last widget. */
	if(_Focus->Index < 0 || _Focus->Index >= _Parent->ChildrenSz) {
		new_stack:
		if(_Parent->Parent != NULL) {
			_Temp = _Focus;

			_Focus = _Focus->Prev;
			_Parent = _Focus->Parent;
			_Change = _Focus->Parent->FocusChange;
			free(_Temp);
			goto change;
		} else {
			if(_Pos < 0)
				_Focus->Index = _Parent->ChildrenSz - 1;
			else
				_Focus->Index = 0;
			if(_Parent->Children[_Focus->Index] != NULL && _Parent->Children[_Focus->Index]->CanFocus != 0)
				--_Change;
		}
	}
	/* Index is now valid decrement the remaining indexes. */
	if(_Change > 0)
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
	return _Focus;
}

void Events(void) {
	int i;
	SDL_Event _Event;

	while(SDL_PollEvent(&_Event) != 0) {
		if(_Event.type == SDL_KEYUP) {
			struct Widget* _Widget = g_Focus->Parent->Children[g_Focus->Index];
			if(_Event.key.keysym.sym == SDLK_w || _Event.key.keysym.sym == SDLK_UP) {
				_Widget->OnUnfocus(_Widget);
				g_Focus = ChangeFocus(g_Focus, -_Widget->Parent->FocusChange);
				g_Focus->Parent->Children[g_Focus->Index]->OnFocus(g_Focus->Parent->Children[g_Focus->Index]);
			} else if(_Event.key.keysym.sym == SDLK_s || _Event.key.keysym.sym == SDLK_DOWN) {
				_Widget->OnUnfocus(_Widget);
				g_Focus = ChangeFocus(g_Focus, _Widget->Parent->FocusChange);
				g_Focus->Parent->Children[g_Focus->Index]->OnFocus(g_Focus->Parent->Children[g_Focus->Index]);
			}
		for(i = 0; i < g_GUIEvents->Size; ++i)
			/* FIXME: If the Lua references are ever out of order because an event is deleted than this loop will fail. */
			if(g_GUIEvents->Events[i].WidgetId == g_Focus->Id && KeyEventCmp(&g_GUIEvents->Events[i].Event, &_Event) == 0) {
				LuaCallEvent(g_LuaState, g_GUIEvents->Events[i].RefId);
				if(g_GUIMenuChange != 0) {
					g_GUIMenuChange = 0;
					return;
				}
			}
		}
	}
}

void Draw(void) {
	struct Container* _Screen = NULL;

	if(g_GUIOk == 0)
		return;
	_Screen = GetScreen(g_LuaState);
	SDL_FillRect(g_Surface, NULL, (g_GUIDefs.Background.r << 16) | (g_GUIDefs.Background.g << 8) | (g_GUIDefs.Background.b));
	if(_Screen != NULL)
		_Screen->OnDraw((struct Widget*) _Screen);
	else
		g_GUIOk = 0;
	g_GUIOk &= (SDL_UpdateWindowSurface(g_Window) == 0);
	if(SDL_GetTicks() <= g_GUITimer + 16)
		SDL_Delay(SDL_GetTicks() - g_GUITimer);
	g_GUITimer = SDL_GetTicks();
}

struct TextBox* CreateTextBox(void) {
	return (struct TextBox*) malloc(sizeof(struct TextBox));
}

struct Container* CreateContainer(void) {
	return (struct Container*) malloc(sizeof(struct Container));
}

struct Table* CreateTable(void) {
	return (struct Table*) malloc(sizeof(struct Table));
}

struct GUIEvents* CreateGUIEvents(void) {
	struct GUIEvents* _New = (struct GUIEvents*) malloc(sizeof(struct GUIEvents));
	_New->TblSz = 16;
	_New->Size = 0;
	_New->Events = calloc(_New->TblSz, sizeof(struct WEvent));
	return _New;
}

struct GUIFocus* CreateGUIFocus(void) {
	struct GUIFocus* _New = (struct GUIFocus*) malloc(sizeof(struct GUIFocus));
	_New->Id = 0;
	_New->Index = 0;
	_New->Parent = NULL;
	_New->Prev = NULL;
	return _New;
}

void ConstructWidget(struct Widget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State) {
	int i;
	_Widget->Id = NextGUIId();
	_Widget->Rect.x = _Rect->x;
	_Widget->Rect.y = _Rect->y;
	_Widget->Rect.w = _Rect->w;
	_Widget->Rect.h = _Rect->h;
	_Widget->LuaRef = LuaWidgetRef(_State);
	_Widget->CanFocus = 1;
	_Widget->OnDraw = NULL;
	_Widget->OnFocus = NULL;
	_Widget->OnUnfocus = NULL;
	_Widget->OnDestroy = NULL;
	if(_Parent != NULL) {
		if(_Parent->Children == NULL) {
			_Parent->Children = calloc(2, sizeof(struct Widget*));
			memset(_Parent->Children, 0, sizeof(struct Widget*) * 2);
			_Parent->ChildrenSz = 2;
		} else if(_Parent->ChildCt == _Parent->ChildrenSz) {
			_Parent->Children = realloc(_Parent->Children, sizeof(struct Widget*) * _Parent->ChildrenSz * 2);
			for(i = _Parent->ChildrenSz; i < _Parent->ChildrenSz * 2; ++i)
				_Parent->Children[i] = NULL;
			_Parent->ChildrenSz *= 2;
		}
		_Parent->Children[_Parent->ChildCt++] = _Widget;
		_Widget->Parent = _Parent;
		_Parent->NewChild(_Parent, _Widget);
	} else
		_Widget->Parent = NULL;
}

void ConstructTextBox(struct TextBox* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Surface* _Text, struct Font* _Font) {
	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect, _State);
	_Widget->OnDraw = TextBoxOnDraw;
	_Widget->OnFocus = TextBoxOnFocus;
	_Widget->OnUnfocus = TextBoxOnUnfocus;
	_Widget->OnDestroy = (void(*)(struct Widget*))DestroyTextBox;
	_Widget->OnDraw = TextBoxOnDraw;
	_Widget->SetText = WidgetSetText;
	_Widget->Text = _Text;
	_Widget->Font = _Font;
	++_Font->RefCt;
}

void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin) {

	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect, _State);
	_Widget->Children = NULL;
	_Widget->ChildrenSz = 0;
	_Widget->ChildCt = 0;
	_Widget->OnDraw = (int(*)(struct Widget*))ContainerOnDraw;
	_Widget->OnFocus = (int(*)(struct Widget*))ContainerOnFocus;
	_Widget->OnUnfocus = (int(*)(struct Widget*))ContainerOnUnfocus;
	_Widget->OnDestroy = (void(*)(struct Widget*))DestroyContainer;
	_Widget->OnDraw = (int(*)(struct Widget*))ContainerOnDraw;
	_Widget->NewChild = NULL;
	_Widget->RemChild = StaticRemChild;
	_Widget->Spacing = _Spacing;
	_Widget->Margins.Top = _Margin->Top;
	_Widget->Margins.Left = _Margin->Left;
	_Widget->Margins.Right = _Margin->Right;
	_Widget->Margins.Bottom = _Margin->Bottom;
	_Widget->FocusChange = 1;
}

void ConstructTable(struct Table* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State,
		int _Spacing, const struct Margin* _Margin, int _Columns, int _Rows, struct Font* _Font) {
	int _THeight = TTF_FontHeight(_Font->Font) + _Spacing;
	int _Size = _Rows * _Columns;
	int i;

	_Rect->h = _THeight * _Columns;
	ConstructContainer((struct Container*)_Widget, _Parent, _Rect, _State, _Spacing, _Margin);
	_Widget->ChildrenSz = _Columns * _Rows;
	_Widget->Children = calloc(_Widget->ChildrenSz, sizeof(struct Widget*));
	_Widget->OnDestroy = (void(*)(struct Widget*))DestroyTable;
	_Widget->NewChild = TableNewChild;
	_Widget->RemChild = DynamicRemChild;
	_Widget->Columns = _Columns;
	_Widget->Rows = _Rows;
	_Widget->Font = _Font;
	_Widget->FocusChange = _Rows;
	++_Font->RefCt;
	for(i = 0; i < _Size; ++i)
		_Widget->Children[i] = NULL;
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

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child) {
	int i;
	int _X = _Parent->Margins.Left;
	int _Y = _Parent->Margins.Top;

	for(i = 0; i < _Parent->ChildCt - 1 && _Parent->Children[i] != NULL; ++i) {
		_X += _Parent->Spacing + _Parent->Children[i]->Rect.w + _Parent->Spacing;
		_Y += _Parent->Spacing + _Parent->Children[i]->Rect.h + _Parent->Spacing;
	}
	_Child->Rect.x += _X;
	_Child->Rect.y += _Y;
}

void WidgetSetParent(struct Container* _Parent, struct Widget* _Child) {
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

void DestroyWidget(struct Widget* _Widget) {
	LuaWidgetUnref(g_LuaState, _Widget->LuaRef);
}

void DestroyTextBox(struct TextBox* _Text) {
	DestroyFont(_Text->Font);
	SDL_FreeSurface(_Text->Text);
	DestroyWidget((struct Widget*)_Text);
}
void DestroyContainer(struct Container* _Container) {
	int i;

	for(i = 0; i < _Container->ChildrenSz; ++i) {
		if(_Container->Children[i] == NULL)
			continue;
		_Container->Children[i]->OnDestroy(_Container->Children[i]);
	}
	DestroyWidget((struct Widget*)_Container);
	free(_Container);
}

void DestroyTable(struct Table* _Table) {
	DestroyFont(_Table->Font);
	DestroyContainer((struct Container*)_Table);
}

int ContainerOnDraw(struct Container* _Container) {
	struct Widget* _Widget = NULL;
	int i;
	int _Ret = 0;

	if(_Container->Children == NULL)
		return 1;
	for(i = 0; i < _Container->ChildCt; ++i) {
		_Widget = _Container->Children[i];
		if(_Widget->Rect.x >= _Container->Rect.x && _Widget->Rect.y >= _Container->Rect.y
				&& _Widget->Rect.x + _Widget->Rect.w <= _Container->Rect.x + _Container->Rect.w
				&& _Widget->Rect.y + _Widget->Rect.h <= _Container->Rect.y + _Container->Rect.h) {
			_Ret = _Widget->OnDraw(_Widget);
			if(_Ret == 0)
				return 0;
		}
	}
	return 1;
}

int ContainerOnFocus(struct Container* _Container) {
	int _FocusIndex = FirstFocusable(_Container);

	if(_FocusIndex != -1) {
		struct GUIFocus* _Focus = NULL;

		if(_Container->Children[_FocusIndex]->OnFocus(_Container->Children[_FocusIndex]) == 0)
			return 0;
		_Focus = malloc(sizeof(struct GUIFocus));
		_Focus->Parent = _Container;
		_Focus->Index = _FocusIndex;
		_Focus->Id = _Container->Children[_FocusIndex]->Id;
		_Focus->Prev = g_Focus;
		g_Focus = _Focus;
	}
	return 1;
}

int ContainerOnUnfocus(struct Container* _Container) {
	if(g_Focus->Parent == _Container && _Container->Children[g_Focus->Index] != NULL) {
		struct GUIFocus* _Focus = g_Focus;

		if(_Container->Children[g_Focus->Index]->OnUnfocus(_Container->Children[g_Focus->Index]) == 0)
			return 0;
		g_Focus = g_Focus->Prev;
		free(_Focus);
	}
	return 1;
}

void VertConNewChild(struct Container* _Parent, struct Widget* _Child) {
	ContainerPosChild(_Parent, _Child);
	_Child->Rect.x = _Parent->Rect.x;
}

void HorzConNewChild(struct Container* _Parent, struct Widget* _Child) {
	ContainerPosChild(_Parent, _Child);
	_Child->Rect.y = _Parent->Rect.y;
}

void StaticRemChild(struct Container* _Parent, struct Widget* _Child) {
	int i;

	for(i = 0; i < _Parent->ChildrenSz; ++i) {
		if(_Parent->Children[i] == _Child) {
			_Parent->Children[i] = NULL;
			--_Parent->ChildCt;
			break;
		}
	}
}

void DynamicRemChild(struct Container* _Parent, struct Widget* _Child) {
	int i;

	for(i = 0; i < _Parent->ChildrenSz; ++i) {
		if(_Parent->Children[i] == _Child) {
			_Parent->Children[i] = NULL;
			--_Parent->ChildCt;
		for(i = i + 1; i < _Parent->ChildrenSz; ++i) {
			_Parent->Children[i - 1] = _Parent->Children[i];
		}
		_Parent->Children[i] = NULL;
		break;
		}
	}
}

int TextBoxOnDraw(struct Widget* _Widget) {
	if(SDL_BlitSurface(((struct TextBox*)_Widget)->Text, NULL, g_Surface, &_Widget->Rect) != 0)
		return 0;
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

void TableNewChild(struct Container* _Parent, struct Widget* _Child) {
	int _Row = _Parent->ChildCt - 1;
	int _Col = 0;

	while(_Row >= ((struct Table*)_Parent)->Rows) {
		_Row -= ((struct Table*)_Parent)->Rows;
		++_Col;
	}
	_Child->Rect.x = _Parent->Rect.x + (_Row * ((struct Table*)_Parent)->CellMax.w);
	_Child->Rect.y = _Parent->Rect.y + (_Col * ((struct Table*)_Parent)->CellMax.h);
	_Child->Rect.w = ((struct Table*)_Parent)->CellMax.w;
	_Child->Rect.h = ((struct Table*)_Parent)->CellMax.h;
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
	SDL_Surface* _Temp = SDL_ConvertSurface(_Surface, g_Surface->format, 0);

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
