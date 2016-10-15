/*
 * File: GuiAux.c
 * Author: David Brotz
 */

#include "GuiAux.h"

#include "AABB.h"

#include "../World.h"

#include <stdlib.h>
#include <assert.h>
#include <SDL2/SDL_ttf.h>

struct Label* CreateLabel(void) {
	return (struct Label*) malloc(sizeof(struct Label));
}

void ConstructLabel(struct Label* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font) {
	SDL_Color _Out;
	SDL_Color _Src = {255, 255, 255, 255};

	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect, _State);
	_Widget->Widget.OnDraw = LabelOnDraw;
	_Widget->Widget.OnDestroy = (void(*)(struct Widget*, lua_State*))DestroyLabel;
	_Widget->SetText = LabelSetText;
	_Widget->Text = _Text;
	SDL_SetTextureBlendMode(_Text, SDL_BLENDMODE_ADD);
	GetBlendValue(&_Src, &g_GUIDefs.FontUnfocus, &_Out);
	SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, _Out.r, _Out.b, _Out.g);
	//SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, g_GUIDefs.FontUnfocus.r, g_GUIDefs.FontUnfocus.b, g_GUIDefs.FontUnfocus.g);
}

void DestroyLabel(struct Label* _Text, lua_State* _State) {
	SDL_DestroyTexture(_Text->Text);
	DestroyWidget((struct Widget*)_Text, _State);
}

int LabelOnDraw(struct Widget* _Widget) {
	SDL_Rect _Rect = {0};

	if(_Widget->IsVisible == 0)
		return 1;
	SDL_QueryTexture(((struct Label*)_Widget)->Text, NULL, NULL, &_Rect.w, &_Rect.h);
	_Rect.x = _Widget->Rect.x + ((_Widget->Rect.w - _Rect.w) / 2);
	_Rect.y = _Widget->Rect.y + ((_Widget->Rect.h - _Rect.h) / 2);
	if(SDL_RenderCopy(g_Renderer, ((struct Label*)_Widget)->Text, NULL, &_Rect) != 0)
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
	_Widget->Widget.OnDraw = ButtonOnDraw;
	_Widget->Background.r = 0x80;
	_Widget->Background.g = 0x80;
	_Widget->Background.b = 0x80;
	_Widget->Background.a = 0xFF;
	_Widget->Widget.OnFocus = ButtonOnFocus;
	_Widget->Widget.OnUnfocus = ButtonOnUnFocus;
	return _Widget;
}

struct Widget* ButtonOnFocus(struct Widget* _Widget, const SDL_Point* _Point) {
	SDL_Color _Out;

	if(PointInAABB(_Point, &_Widget->Rect) == SDL_FALSE || _Widget->CanFocus == 0) {
		return NULL;
	}
	GetBlendValue(&g_GUIDefs.FontUnfocus, &g_GUIDefs.FontFocus, &_Out);
	SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, _Out.r, _Out.b, _Out.g);
	return _Widget;
}

int ButtonOnUnFocus(struct Widget* _Widget) {
	SDL_SetTextureColorMod(((struct Label*)_Widget)->Text, g_GUIDefs.FontUnfocus.r, g_GUIDefs.FontUnfocus.b, g_GUIDefs.FontUnfocus.g);
	return 1;
}

int ButtonOnDraw(struct Widget* _Widget) {
	struct Button* _Button = (struct Button*) _Widget;

	SDL_SetRenderDrawColor(g_Renderer, _Button->Background.r, _Button->Background.b, _Button->Background.g, _Button->Background.a);
	SDL_RenderFillRect(g_Renderer, &_Widget->Rect);
	return LabelOnDraw(_Widget);
}

void ButtonSetClickable(struct Button* _Button, int _Clickable) {
	if(_Clickable == 1) {
		_Button->Widget.OnDraw = ButtonOnDraw;
		_Button->Widget.Clickable  = 1;
	} else {
		_Button->Widget.OnDraw = LabelOnDraw;
		_Button->Widget.Clickable  = 0;
	}
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
	_Widget->Container.HorzFocChange = TableHorzFocChange;
	_Widget->Container.ChildrenSz = _Columns * _Rows;
	_Widget->Container.Children = calloc(_Widget->Container.ChildrenSz, sizeof(struct Widget*));
	_Widget->Container.Widget.OnDestroy = (void(*)(struct Widget*, lua_State*))DestroyTable;
	_Widget->Container.NewChild = TableNewChild;
	_Widget->Container.RemChild = DynamicRemChild;
	_Widget->Columns = _Columns;
	_Widget->Rows = _Rows;
	_Widget->Container.VertFocChange = _Rows;
	_Widget->CellMax.w = 32;
	_Widget->CellMax.h = 32;
	++_Font->RefCt;
	for(i = 0; i < _Size; ++i)
		_Widget->Container.Children[i] = NULL;
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
	_Child->Rect.x = _Parent->Widget.Rect.x + (_Row * ((struct Table*)_Parent)->CellMax.w);
	_Child->Rect.y = _Parent->Widget.Rect.y + (_Col * ((struct Table*)_Parent)->CellMax.h);
	if(_Child->Rect.w > ((struct Table*)_Parent)->CellMax.w)
		_Child->Rect.w = ((struct Table*)_Parent)->CellMax.w;
	if(_Child->Rect.h > ((struct Table*)_Parent)->CellMax.h)
		_Child->Rect.h = ((struct Table*)_Parent)->CellMax.h;
}

int TableHorzFocChange(const struct Container* _Container) {
	return ((struct Table*)_Container)->Columns;
}

struct TextBox* CreateTextBox(void) {
	return (struct TextBox*) malloc(sizeof(struct TextBox));
}

void DestroyTextBox(struct TextBox* _Widget, lua_State* _State) {
	SDL_DestroyTexture(_Widget->TextSurface);
	DestroyWidget((struct Widget*) _Widget, _State);
}

void ConstructTextBox(struct TextBox* _TextBox, struct Container* _Parent, int _Rows, int _Chars, lua_State* _State, struct Font* _Font) {
	SDL_Rect _Rect = {
		0,
		0,
		_Chars * 8,
		16
	};
	ConstructWidget((struct Widget*)_TextBox, _Parent, &_Rect, _State); 
	_TextBox->TextSurface = NULL;
	ConstructLinkedList(&_TextBox->Letters);
	_TextBox->Widget.OnDraw = (GuiCallWidget) TextBoxOnDraw;
	_TextBox->Widget.OnKey = (GuiOnKey) TextBoxOnKey;
	_TextBox->Widget.OnDestroy = (GuiCallDestroy) DestroyTextBox;
}

void TextBoxOnKey(struct TextBox* _Widget, unsigned int _Key, unsigned int _Mod) {
	char _Buffer[_Widget->Letters.Size + 1];
	struct LnkLst_Node* _Itr = NULL;
	int i = 0;

	if(_Key < SDLK_SPACE || _Key > SDLK_KP_RIGHTBRACE) {
		if(_Key == SDLK_BACKSPACE) {
			LnkLstPopBack(&_Widget->Letters);
		} else {
			return;
		}
	} else {
		LnkLstPushBack(&_Widget->Letters,(void*)(_Key));
	}
	_Itr = _Widget->Letters.Front;
	SDL_DestroyTexture(_Widget->TextSurface);
	while(_Itr != NULL) {
		_Buffer[i++] = (int) _Itr->Data;
		_Itr = _Itr->Next;
	}
	_Buffer[i] = '\0';
	assert(i == _Widget->Letters.Size);
	_Widget->TextSurface = SurfaceToTexture(TTF_RenderText_Solid(g_GUIFonts->Font, _Buffer, g_GUIDefs.FontUnfocus));
	SDL_QueryTexture(_Widget->TextSurface, NULL, NULL, &_Widget->TextRect.w, &_Widget->TextRect.h);
	if(_Widget->TextRect.w> _Widget->Widget.Rect.w)
		_Widget->TextRect.w= _Widget->Widget.Rect.w;
	if(_Widget->TextRect.h> _Widget->Widget.Rect.h)
		_Widget->TextRect.h= _Widget->Widget.Rect.h;
	_Widget->TextRect.x = _Widget->Widget.Rect.x;
	_Widget->TextRect.y = _Widget->Widget.Rect.y;
}

int TextBoxOnDraw(struct TextBox* _Widget) {
	return SDL_RenderCopy(g_Renderer, _Widget->TextSurface, NULL, &_Widget->TextRect);
}

struct ContextItem* CreateContextItem(void) {
	return (struct ContextItem*) malloc(sizeof(struct ContextItem));
}

void ConstructContextItem(struct ContextItem* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin) {
	ConstructContainer((struct Container*)_Widget, _Parent, _Rect, _State, _Spacing, _Margin);
	_Widget->Container.Widget.OnDraw = (int(*)(struct Widget*))ContextItemOnDraw;
	_Widget->Container.Widget.OnFocus = (struct Widget* (*)(struct Widget*, const SDL_Point*))ContextItemOnFocus;
	_Widget->Container.Widget.OnUnfocus = (int(*)(struct Widget*))ContextItemOnUnfocus;
	_Widget->Container.NewChild = ContextItemNewChild;
	_Widget->Container.VertFocChange = 0;
	_Widget->Container.HorzFocChange = ContextHorzFocChange;
	_Widget->ShowContexts = 0;
}

int ContextItemOnDraw(struct ContextItem* _Container) {
	if(_Container->ShowContexts != 0) {
		for(int i = 1; i < _Container->Container.ChildCt; ++i)
			_Container->Container.Children[i]->OnDraw(_Container->Container.Children[i]);
	}
	if(_Container->Container.ChildCt > 0)
		_Container->Container.Children[0]->OnDraw(_Container->Container.Children[0]);
	return 1;
}

struct Widget* ContextItemOnFocus(struct ContextItem* _Widget, const SDL_Point* _Point) {
	ContainerOnFocus((struct Container*)_Widget, _Point);
	_Widget->ShowContexts = 1;
	_Widget->Container.VertFocChange = _Widget->Container.ChildCt;
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

/*struct GameWorldWidget* CreateGWWidget(void) {
	return (struct GameWorldWidget*) malloc(sizeof(struct GameWorldWidget));
}

struct GameWorldWidget* ConstructGWWidget(struct GameWorldWidget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, struct GameWorld* _World) {
	ConstructWidget((struct Widget*) _Widget, _Parent, _Rect, _State);
	_Widget->OnDraw = GameWorldWidgetOnDraw;
	_Widget->World = _World;
	return _Widget;
}

int GameWorldWidgetOnDraw(struct Widget* _Widget) {
	GameWorldDraw(((struct GameWorldWidget*) _Widget)->World);
	return 1;
}*/
