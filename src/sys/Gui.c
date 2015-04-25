/*
 * File: Gui.c
 * Author: David Brotz
 */

#include "Gui.h"

#include "Video.h"
#include "Stack.h"
#include "GuiLua.h"

#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

struct GUIDef g_GUIDefs = {NULL, {255, 255, 255, 255}, {128, 128, 128, 255}};
int g_GUIMenuChange = 0;
int g_GUIId = 0;
struct GUIFocus* g_Focus = NULL;
struct GUIEvents* g_GUIEvents = NULL;
struct Font* g_GUIFonts = NULL;
struct Stack g_GUIStack = {NULL, 0};

int NextGUIId(void) {return g_GUIId++;}

struct Label* CreateLabel(void) {
	return (struct Label*) malloc(sizeof(struct Label));
}

struct Container* CreateContainer(void) {
	return (struct Container*) malloc(sizeof(struct Container));
}

struct Table* CreateTable(void) {
	return (struct Table*) malloc(sizeof(struct Table));
}

struct ContextItem* CreateContextItem(void) {
	return (struct ContextItem*) malloc(sizeof(struct ContextItem));
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
	_Widget->OnKeyUp = WidgetOnKeyUp;
	_Widget->OnDestroy = NULL;
	if(_Parent != NULL)
		WidgetSetParent(_Parent, _Widget);
	else
		_Widget->Parent = NULL;
}

void ConstructLabel(struct Label* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font) {
	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect, _State);
	_Widget->OnDraw = LabelOnDraw;
	_Widget->OnFocus = LabelOnFocus;
	_Widget->OnUnfocus = LabelOnUnfocus;
	_Widget->OnDestroy = (void(*)(struct Widget*, lua_State*))DestroyLabel;
	_Widget->OnDraw = LabelOnDraw;
	_Widget->SetText = WidgetSetText;
	_Widget->Text = _Text;
	SDL_SetTextureBlendMode(_Text, SDL_BLENDMODE_ADD);
	SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, g_GUIDefs.FontUnfocus.r, g_GUIDefs.FontUnfocus.b, g_GUIDefs.FontUnfocus.g);
}

void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin) {
	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect, _State);
	_Widget->Children = NULL;
	_Widget->ChildrenSz = 0;
	_Widget->ChildCt = 0;
	_Widget->OnDraw = (int(*)(struct Widget*))ContainerOnDraw;
	_Widget->OnFocus = (int(*)(struct Widget*))ContainerOnFocus;
	_Widget->OnUnfocus = (int(*)(struct Widget*))ContainerOnUnfocus;
	_Widget->OnDestroy = (void(*)(struct Widget*, lua_State*))DestroyContainer;
	_Widget->OnDraw = (int(*)(struct Widget*))ContainerOnDraw;
	_Widget->NewChild = NULL;
	_Widget->RemChild = StaticRemChild;
	_Widget->Spacing = _Spacing;
	_Widget->Margins.Top = _Margin->Top;
	_Widget->Margins.Left = _Margin->Left;
	_Widget->Margins.Right = _Margin->Right;
	_Widget->Margins.Bottom = _Margin->Bottom;
	_Widget->VertFocChange = 1;
	_Widget->HorzFocChange = ContainerHorzFocChange;
}

void ConstructContextItem(struct ContextItem* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin) {
	ConstructContainer((struct Container*)_Widget, _Parent, _Rect, _State, _Spacing, _Margin);
	_Widget->OnDraw = (int(*)(struct Widget*))ContextItemOnDraw;
	_Widget->OnFocus = (int(*)(struct Widget*))ContextItemOnFocus;
	_Widget->OnUnfocus = (int(*)(struct Widget*))ContextItemOnUnfocus;
	_Widget->NewChild = ContextItemNewChild;
	_Widget->VertFocChange = 0;
	_Widget->HorzFocChange = ContextHorzFocChange;
	_Widget->ShowContexts = 0;
}

void ConstructTable(struct Table* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State,
		int _Spacing, const struct Margin* _Margin, int _Columns, int _Rows, struct Font* _Font) {
	int _THeight = TTF_FontHeight(_Font->Font) + _Spacing;
	int _Size = _Rows * _Columns;
	int i;

	_Rect->h = _THeight * _Columns;
	ConstructContainer((struct Container*)_Widget, _Parent, _Rect, _State, _Spacing, _Margin);
	_Widget->HorzFocChange = TableHorzFocChange;
	_Widget->ChildrenSz = _Columns * _Rows;
	_Widget->Children = calloc(_Widget->ChildrenSz, sizeof(struct Widget*));
	_Widget->OnDestroy = (void(*)(struct Widget*, lua_State*))DestroyTable;
	_Widget->NewChild = TableNewChild;
	_Widget->RemChild = DynamicRemChild;
	_Widget->Columns = _Columns;
	_Widget->Rows = _Rows;
	_Widget->VertFocChange = _Rows;
	++_Font->RefCt;
	for(i = 0; i < _Size; ++i)
		_Widget->Children[i] = NULL;
}

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child) {
	int i;
	int _X = _Parent->Margins.Left + _Parent->Rect.x;
	int _Y = _Parent->Margins.Top + _Parent->Rect.y;

	for(i = 0; i < _Parent->ChildCt - 1 && _Parent->Children[i] != NULL; ++i) {
		_X += _Parent->Spacing + _Parent->Children[i]->Rect.w + _Parent->Spacing;
		_Y += _Parent->Spacing + _Parent->Children[i]->Rect.h + _Parent->Spacing;
	}
	_Child->Rect.x += _X;
	_Child->Rect.y += _Y;
}

void WidgetSetParent(struct Container* _Parent, struct Widget* _Child) {
	int i = 0;

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
	_Parent->Children[_Parent->ChildCt++] = _Child;
	_Child->Parent = _Parent;
	_Parent->NewChild(_Parent, _Child);
}

void WidgetOnKeyUp(struct Widget* _Widget, SDL_KeyboardEvent* _Event) {

}

void DestroyWidget(struct Widget* _Widget, lua_State* _State) {
	LuaWidgetUnref(_State, _Widget->LuaRef);
}

void DestroyLabel(struct Label* _Text, lua_State* _State) {
	SDL_DestroyTexture(_Text->Text);
	DestroyWidget((struct Widget*)_Text, _State);
}
void DestroyContainer(struct Container* _Container, lua_State* _State) {
	int i;

	for(i = 0; i < _Container->ChildrenSz; ++i) {
		if(_Container->Children[i] == NULL)
			continue;
		_Container->Children[i]->OnDestroy(_Container->Children[i], _State);
	}
	DestroyWidget((struct Widget*)_Container, _State);
	free(_Container);
}

void DestroyTable(struct Table* _Table, lua_State* _State) {
	DestroyContainer((struct Container*)_Table, _State);
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
	/*if(g_Focus->Parent == _Container && _Container->Children[g_Focus->Index] != NULL) {
		struct GUIFocus* _Focus = g_Focus;

		g_Focus = g_Focus->Prev;
		free(_Focus);
	}*/
	return 1;
}

int ContainerHorzFocChange(const struct Container* _Container) {
	return 0;
}

void VertConNewChild(struct Container* _Parent, struct Widget* _Child) {
	ContainerPosChild(_Parent, _Child);
	_Child->Rect.x = _Parent->Rect.x;
}

void HorzConNewChild(struct Container* _Parent, struct Widget* _Child) {
	ContainerPosChild(_Parent, _Child);
	_Child->Rect.y = _Parent->Rect.y;
}

void ContextItemNewChild(struct Container* _Parent, struct Widget* _Child) {
	if(_Parent->Children[0] == _Child) {
		_Child->Rect.x = _Parent->Rect.x;
		_Child->Rect.y = _Parent->Rect.y;
		return;
	}
	ContainerPosChild(_Parent, _Child);
	_Child->Rect.x = _Parent->Children[0]->Rect.w + _Parent->Spacing;
	_Child->Rect.y = _Child->Rect.y - _Parent->Children[0]->Rect.h;
	_Parent->VertFocChange = _Parent->ChildCt;
}

int ContextItemOnDraw(struct ContextItem* _Container) {
	int i = 0;

	if(_Container->ShowContexts != 0) {
		for(i = 1; i < _Container->ChildCt; ++i)
			_Container->Children[i]->OnDraw(_Container->Children[i]);
	}
	if(_Container->ChildCt > 0)
		_Container->Children[0]->OnDraw(_Container->Children[0]);
	return 1;
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

int LabelOnDraw(struct Widget* _Widget) {
	if(SDL_RenderCopy(g_Renderer, ((struct Label*)_Widget)->Text, NULL, &_Widget->Rect))
	//if(SDL_UpdateTexture(g_WindowTexture, &_Widget->Rect, ((struct Label*)_Widget)->Text->pixels, ((struct Label*)_Widget)->Text->pitch) != 0)
	//if(SDL_BlitSurface(((struct Label*)_Widget)->Text, NULL, g_WindowSurface, &_Widget->Rect) != 0)
		return 0;
	return 1;
}

int LabelOnFocus(struct Widget* _Widget) {
	SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, g_GUIDefs.FontFocus.r, g_GUIDefs.FontFocus.b, g_GUIDefs.FontFocus.g);
	//ChangeColor(((struct Label*)_Widget)->Text, &g_GUIDefs.FontUnfocus, &g_GUIDefs.FontFocus);
	return 1;
}
int LabelOnUnfocus(struct Widget* _Widget) {
	SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, g_GUIDefs.FontUnfocus.r, g_GUIDefs.FontUnfocus.b, g_GUIDefs.FontUnfocus.g);
	//ChangeColor(((struct Label*)_Widget)->Text, &g_GUIDefs.FontFocus, &g_GUIDefs.FontUnfocus);
	return 1;
}

int WidgetSetText(struct Widget* _Widget, SDL_Texture* _Text) {
	if(((struct Label*)_Widget)->Text != NULL)
		SDL_DestroyTexture(((struct Label*)_Widget)->Text);
	((struct Label*)_Widget)->Text = _Text;
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
	if(_Child->Rect.w > ((struct Table*)_Parent)->CellMax.w)
		_Child->Rect.w = ((struct Table*)_Parent)->CellMax.w;
	if(_Child->Rect.h > ((struct Table*)_Parent)->CellMax.h)
		_Child->Rect.h = ((struct Table*)_Parent)->CellMax.h;
}

int TableHorzFocChange(const struct Container* _Container) {
	return ((struct Table*)_Container)->Columns;
}

void WidgetOnEvent(struct Widget* _Widget, int _RefId, int _Key, int _KeyState, int _KeyMod) {
	SDL_Event _Event;
	struct WEvent _WEvent;

	_Event.type = SDL_KEYUP;
	_Event.key.state = _KeyState;
	_Event.key.keysym.sym = _Key;
	_Event.key.keysym.mod = _KeyMod;
	if(g_GUIEvents->Size == g_GUIEvents->TblSz) {
		g_GUIEvents->Events = realloc(g_GUIEvents->Events, sizeof(SDL_Event) * g_GUIEvents->TblSz * 2);
		g_GUIEvents->TblSz *= 2;
	}
	_WEvent.Event = _Event;
	_WEvent.WidgetId = _Widget->Id;
	_WEvent.RefId = _RefId;
	g_GUIEvents->Events[g_GUIEvents->Size++] = _WEvent;
}

int ContextItemOnFocus(struct ContextItem* _Widget) {
	ContainerOnFocus((struct Container*)_Widget);
	_Widget->ShowContexts = 1;
	_Widget->VertFocChange = _Widget->ChildCt;
	return 1;
}

int ContextItemOnUnfocus(struct ContextItem* _Widget) {
	ContainerOnUnfocus((struct Container*)_Widget);
	_Widget->ShowContexts = 0;
	return 1;
}

int ContextHorzFocChange(const struct Container* _Container) {
	//if(g_Focus->Id == _Container->Children[0]->Id)
	//	return 1;
	return 1;
}
