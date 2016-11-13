/*
 * File: GuiAux.h
 * Author: David Brotz
 */
#ifndef __GUIAUX_H
#define __GUIAUX_H

#include "Gui.h"

#include "../sys/LinkedList.h"

struct GameWorld;

struct Label {
	struct Widget Widget;
	int (*SetText)(struct Widget*, SDL_Texture*);
	SDL_Texture* Text;
};

struct Table {
	struct Container Container;
	int Rows;
	int Columns;
	struct Area CellMax; /* max area of a cell. */
};

struct TextBox {
	struct Widget Widget;
	SDL_Texture* TextSurface;
	struct LinkedList Letters; /* List of letters in the text box. */
	SDL_Rect TextRect;
};

struct ContextItem {
	struct Container Container;
	int ShowContexts;
};

struct Button {
	struct Widget Widget;
	int (*SetText)(struct Widget*, SDL_Texture*);
	SDL_Texture* Text;
};

/*
struct GameWorldWidget {
	DECLARE_WIDGET;
	const struct GameWorld* World;
};
*/

struct Label* CreateLabel(void);
void ConstructLabel(struct Label* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, const struct GuiStyle* _Style);
void DestroyLabel(struct Label* _Text, lua_State* _State);
int LabelOnDraw(struct Widget* _Widget);
int LabelSetText(struct Widget* _Widget, SDL_Texture* _Text);

struct Button* CreateButton(void);
struct Button* ConstructButton(struct Button* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font);
struct Widget* ButtonOnFocus(struct Widget* _Widget, const SDL_Point* _Point);
int ButtonOnUnFocus(struct Widget* _Widget);
int ButtonOnDraw(struct Widget* _Widget);
void ButtonSetClickable(struct Button* _Button, int _Clickable);

struct Table* CreateTable(void);
void ConstructTable(struct Table* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State,
		int _Spacing, int _Columns, int _Rows, struct Font* _Font);
void DestroyTable(struct Table* _Table, lua_State* _State);
void TableNewChild(struct Container* _Parent, struct Widget* _Child);
int TableHorzFocChange(const struct Container* _Container);

struct ContextItem* CreateContextItem(void);
void ConstructContextItem(struct ContextItem* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing);
int ContextItemOnDraw(struct ContextItem* _Container);
struct Widget* ContextItemOnFocus(struct ContextItem* _Widget, const SDL_Point* _Point);
int ContextItemOnUnfocus(struct ContextItem* _Widget);
int ContextHorzFocChange(const struct Container* _Container);

struct TextBox* CreateTextBox(void);
void ConstructTextBox(struct TextBox* _TextBox, struct Container* _Parent, int _Rows, int _Chars, lua_State* _State, struct Font* _Font);
void TextBoxOnKey(struct TextBox* _TextBox, unsigned int _Key, unsigned int _Mod); 
int TextBoxOnDraw(struct TextBox* _Widget);

/*struct GameWorldWidget* CreateGWWidget(void);
struct GameWorldWidget* ConstructGWWidget(struct GameWorldWidget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, struct GameWorld* _World);
int GameWorldWidgetOnDraw(struct Widget* _Widget);*/

#endif
