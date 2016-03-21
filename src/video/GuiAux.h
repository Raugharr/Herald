/*
 * File: GuiAux.h
 * Author: David Brotz
 */
#ifndef __GUIAUX_H
#define __GUIAUX_H

#include "Gui.h"

struct GameWorld;

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

/*
struct GameWorldWidget {
	DECLARE_WIDGET;
	const struct GameWorld* World;
};
*/

struct Label* CreateLabel(void);
void ConstructLabel(struct Label* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font);
void DestroyLabel(struct Label* _Text, lua_State* _State);
int LabelOnDraw(struct Widget* _Widget);
int LabelSetText(struct Widget* _Widget, SDL_Texture* _Text);

struct Button* CreateButton(void);
struct Button* ConstructButton(struct Button* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font);
struct Widget* ButtonOnFocus(struct Widget* _Widget, const SDL_Point* _Point);
int ButtonOnUnFocus(struct Widget* _Widget);
int ButtonOnDraw(struct Widget* _Widget);

struct Table* CreateTable(void);
void ConstructTable(struct Table* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State,
		int _Spacing, const struct Margin* _Margin, int _Columns, int _Rows, struct Font* _Font);
void DestroyTable(struct Table* _Table, lua_State* _State);
void TableNewChild(struct Container* _Parent, struct Widget* _Child);
int TableHorzFocChange(const struct Container* _Container);

struct ContextItem* CreateContextItem(void);
void ConstructContextItem(struct ContextItem* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin);
int ContextItemOnDraw(struct ContextItem* _Container);
struct Widget* ContextItemOnFocus(struct ContextItem* _Widget, const SDL_Point* _Point);
int ContextItemOnUnfocus(struct ContextItem* _Widget);
int ContextHorzFocChange(const struct Container* _Container);

/*struct GameWorldWidget* CreateGWWidget(void);
struct GameWorldWidget* ConstructGWWidget(struct GameWorldWidget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, struct GameWorld* _World);
int GameWorldWidgetOnDraw(struct Widget* _Widget);*/

#endif
