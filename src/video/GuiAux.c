/*
 * File: GuiAux.c
 * Author: David Brotz
 */

#include "GuiAux.h"

#include "AABB.h"

#include <stdlib.h>

#include <SDL2/SDL_ttf.h>

struct Label* CreateLabel(void) {
	return (struct Label*) malloc(sizeof(struct Label));
}

void ConstructLabel(struct Label* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font) {
	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect, _State);
	_Widget->OnDraw = LabelOnDraw;
	_Widget->OnDestroy = (void(*)(struct Widget*, lua_State*))DestroyLabel;
	_Widget->OnDraw = LabelOnDraw;
	_Widget->SetText = LabelSetText;
	_Widget->Text = _Text;
	SDL_SetTextureBlendMode(_Text, SDL_BLENDMODE_ADD);
	SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, g_GUIDefs.FontUnfocus.r, g_GUIDefs.FontUnfocus.b, g_GUIDefs.FontUnfocus.g);
}

void DestroyLabel(struct Label* _Text, lua_State* _State) {
	SDL_DestroyTexture(_Text->Text);
	DestroyWidget((struct Widget*)_Text, _State);
}

int LabelOnDraw(struct Widget* _Widget) {
	if(_Widget->IsVisible == 0)
		return 1;
	if(SDL_RenderCopy(g_Renderer, ((struct Label*)_Widget)->Text, NULL, &_Widget->Rect) != 0)
		return 0;
	return 1;
}

int LabelSetText(struct Widget* _Widget, SDL_Texture* _Text) {
	if(((struct Label*)_Widget)->Text != NULL)
		SDL_DestroyTexture(((struct Label*)_Widget)->Text);
	((struct Label*)_Widget)->Text = _Text;
	return 1;
}

struct Button* CreateButton(void) {
	return (struct Button*) malloc(sizeof(struct Button));
}

struct Button* ConstructButton(struct Button* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font) {
	ConstructLabel((struct Label*)_Widget, _Parent, _Rect, _State, _Text, _Font); //Only Temporary
	_Widget->OnDraw = ButtonOnDraw;
	_Widget->Background.r = 65;
	_Widget->Background.g = 48;
	_Widget->Background.b = 19;
	_Widget->Background.a = 0xFF;
	_Widget->OnFocus = ButtonOnFocus;
	_Widget->OnUnfocus = ButtonOnUnFocus;
	return _Widget;
}

struct Widget* ButtonOnFocus(struct Widget* _Widget, const SDL_Point* _Point) {
	if(PointInAABB(_Point, &_Widget->Rect) == SDL_FALSE || _Widget->CanFocus == 0) {
		return NULL;
	}
	SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, g_GUIDefs.FontFocus.r, g_GUIDefs.FontFocus.b, g_GUIDefs.FontFocus.g);
	return _Widget;
}

int ButtonOnUnFocus(struct Widget* _Widget) {
	SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, g_GUIDefs.FontUnfocus.r, g_GUIDefs.FontUnfocus.b, g_GUIDefs.FontUnfocus.g);
	return 1;
}

int ButtonOnDraw(struct Widget* _Widget) {
	struct Button* _Button = (struct Button*) _Widget;

	SDL_SetRenderDrawColor(g_Renderer, _Button->Background.r, _Button->Background.g, _Button->Background.b, _Button->Background.a);
	SDL_RenderFillRect(g_Renderer, &_Widget->Rect);
	return LabelOnDraw(_Widget);
}
struct Table* CreateTable(void) {
	return (struct Table*) malloc(sizeof(struct Table));
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
	_Widget->CellMax.w = 32;
	_Widget->CellMax.h = 32;
	++_Font->RefCt;
	for(i = 0; i < _Size; ++i)
		_Widget->Children[i] = NULL;
}

void DestroyTable(struct Table* _Table, lua_State* _State) {
	DestroyContainer((struct Container*)_Table, _State);
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

struct ContextItem* CreateContextItem(void) {
	return (struct ContextItem*) malloc(sizeof(struct ContextItem));
}

void ConstructContextItem(struct ContextItem* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin) {
	ConstructContainer((struct Container*)_Widget, _Parent, _Rect, _State, _Spacing, _Margin);
	_Widget->OnDraw = (int(*)(struct Widget*))ContextItemOnDraw;
	_Widget->OnFocus = (struct Widget* (*)(struct Widget*, const SDL_Point*))ContextItemOnFocus;
	_Widget->OnUnfocus = (int(*)(struct Widget*))ContextItemOnUnfocus;
	_Widget->NewChild = ContextItemNewChild;
	_Widget->VertFocChange = 0;
	_Widget->HorzFocChange = ContextHorzFocChange;
	_Widget->ShowContexts = 0;
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

struct Widget* ContextItemOnFocus(struct ContextItem* _Widget, const SDL_Point* _Point) {
	ContainerOnFocus((struct Container*)_Widget, _Point);
	_Widget->ShowContexts = 1;
	_Widget->VertFocChange = _Widget->ChildCt;
	return (struct Widget*) _Widget;
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
