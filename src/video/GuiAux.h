/*
 * File: GuiAux.h
 * Author: David Brotz
 */
#ifndef __GUIAUX_H
#define __GUIAUX_H

#include "Gui.h"

struct Label {
	DECLARE_WIDGET;
	int (*SetText)(struct Widget*, SDL_Texture*);
	SDL_Texture* Text;
};

struct Table {
	DECLARE_CONTAINER;
	int Rows;
	int Columns;
	struct Area CellMax; /* max area of a cell. */
};

struct ContextItem {
	DECLARE_CONTAINER;
	int ShowContexts;
};

struct Button {
	DECLARE_WIDGET;
	int (*SetText)(struct Widget*, SDL_Texture*);
	SDL_Texture* Text;
	SDL_Color Background;
};

struct Label* CreateLabel(void);
struct Button* CreateButton(void);
struct Table* CreateTable(void);
struct ContextItem* CreateContextItem(void);

void ConstructLabel(struct Label* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font);
struct Button* ConstructButton(struct Button* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font);
void ConstructContextItem(struct ContextItem* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin);
void ConstructTable(struct Table* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State,
		int _Spacing, const struct Margin* _Margin, int _Columns, int _Rows, struct Font* _Font);

void DestroyLabel(struct Label* _Text, lua_State* _State);
void DestroyTable(struct Table* _Table, lua_State* _State);

int ContextItemOnDraw(struct ContextItem* _Container);
struct Widget* ContextItemOnFocus(struct ContextItem* _Widget, const SDL_Point* _Point);
int ContextItemOnUnfocus(struct ContextItem* _Widget);
int ContextHorzFocChange(const struct Container* _Container);

int LabelOnDraw(struct Widget* _Widget);
struct Widget* LabelOnFocus(struct Widget* _Widget, const SDL_Point* _Point);
int LabelOnUnfocus(struct Widget* _Widget);
int LabelSetText(struct Widget* _Widget, SDL_Texture* _Text);

int ButtonOnDraw(struct Widget* _Widget);

void TableNewChild(struct Container* _Parent, struct Widget* _Child);
int TableHorzFocChange(const struct Container* _Container);

#endif
