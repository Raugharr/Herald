/*
 * File: GuiAux.h
 * Author: David Brotz
 */
#ifndef __GUIAUX_H
#define __GUIAUX_H

#include "Gui.h"

#include "TextRenderer.h"

#include "../sys/LinkedList.h"
#include "../sys/Array.h"

struct Label {
	struct Widget Widget;
	SDL_Rect LabelPos;
	void (*SetText)(struct Widget*, const char*);
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
	struct TextStream Stream;
	SDL_Rect TextRect;
};

struct ContextItem {
	struct Container Container;
	int ShowContexts;
};

struct Button {
	struct Label Label;
};

struct Console {
	struct Container Container;
	struct TextBox TextBox;
	struct Label* History;//Array of labels.
	uint8_t HistorySize;//Number of labels that are in the console.
	uint8_t Top; //Index of Label at top of the stack in the History array.
	struct Array TextArr; //Array of text to fill History with.
};

//NOTE: Do yo really need to pass a Rect to a label and textbox? Shouldn't this be calculated internally to make it easier on the user?
struct Label* CreateLabel(void);
void ConstructLabel(struct Label* Widget, struct Container* Parent, const char* Text);
void DestroyLabel(struct Widget* Widget);
int LabelOnDraw(struct Widget* Widget);
void LabelSetText(struct Widget* Widget, const char* Text);

struct Button* CreateButton(void);
struct Button* ConstructButton(struct Button* Widget, struct Container* Parent, const char* Text);
struct Widget* ButtonOnFocus(struct Widget* Widget, const SDL_Point* Point);
int ButtonOnUnFocus(struct Widget* Widget);
int ButtonOnDraw(struct Widget* Widget);
void ButtonSetClickable(struct Button* Button, int Clickable);

struct Table* CreateTable(void);
void ConstructTable(struct Table* Widget, struct Container* Parent, SDL_Rect* Rect,	int Columns, int Rows);
void DestroyTable(struct Widget* Widget);
void TableNewChild(struct Container* Parent, struct Widget* Child);

struct ContextItem* CreateContextItem(void);
void ConstructContextItem(struct ContextItem* Widget, struct Container* Parent, SDL_Rect* Rect);
int ContextItemOnDraw(struct ContextItem* Container);
struct Widget* ContextItemOnFocus(struct ContextItem* Widget, const SDL_Point* Point);
int ContextItemOnUnfocus(struct ContextItem* Widget);

struct TextBox* CreateTextBox(void);
void ConstructTextBox(struct TextBox* TextBox, struct Container* Parent, int Chars);
void TextBoxOnKey(struct Widget* Widget, const union UWidgetOnKey* Event); 
int TextBoxOnDraw(struct Widget* Widget);
struct Widget* TextBoxOnFocus(struct Widget* Widget, const SDL_Point* Point);
int TextBoxOnUnfocus(struct Widget* Widget);

struct Console* CreateConsole(void);
void ConstructConsole(struct Console* Console, struct Container* Parent, int Rows, int Cols);
void ConsoleOnKey(struct Widget* Widget, const union UWidgetOnKey* Event);

#endif
