/*
 * File: Gui.h
 * Author: David Brotz
 */
#ifndef __GUI_H
#define __GUI_H

#include "Video.h"

typedef struct SDL_Color SDL_Color;
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Rect SDL_Rect;
typedef struct lua_State lua_State;

/*
 * TODO: Remove LuaOnClickFunc and and put function into the Lua table and use WEvents to handle click events.
 */
#define DECLARE_WIDGET														\
	int Id;																	\
	struct Container* Parent;												\
	SDL_Rect Rect;															\
	int IsDraggable;														\
	int LuaRef;																\
	int CanFocus;															\
	int IsVisible;															\
	int LuaOnClickFunc;														\
	int (*OnDraw)(struct Widget*);											\
	struct Widget* (*OnClick)(struct Widget*, const SDL_Point*);			\
	struct Widget* (*OnFocus)(struct Widget*, const SDL_Point*);			\
	void (*OnKeyUp)(struct Widget*, SDL_KeyboardEvent*);					\
	void (*SetPosition)(struct Widget*, const SDL_Point*);					\
	int (*OnUnfocus)(struct Widget*);										\
	void (*OnDebug)(const struct Widget*);									\
	void (*OnDestroy)(struct Widget*, lua_State*)

#define DECLARE_CONTAINER											\
	DECLARE_WIDGET;													\
	void (*NewChild)(struct Container*, struct Widget*);			\
	void (*RemChild)(struct Container*, struct Widget*);			\
	int(*HorzFocChange)(const struct Container*);					\
	struct Widget** Children;										\
	int ChildrenSz;													\
	int ChildCt;													\
	int Spacing;													\
	SDL_Color Background;											\
	int VertFocChange;												\
	struct Margin Margins

struct WEvent {
	struct KeyMouseState Event; //TODO: All the data is checked for an event not just relevant data. This means if we want to click a widget it will fail if we are clicking and pressing a button etc.
	int WidgetId;
	int RefId;
};

struct GUIFocus {
	const struct Container* Parent;
	int Index;
	int Id;
	struct GUIFocus* Prev;
};

struct GUIEvents {
	/* TODO: Events based on clicking a widget are not possible as the hooking widget is not stored. */
	struct WEvent* Events;
	int TblSz;
	int Size;
};

struct GUIDef {
	struct Font* Font;
	SDL_Color FontFocus;
	SDL_Color FontUnfocus;
	SDL_Color Background;
};

extern int g_GUIId;
extern int g_GUIMenuChange;
extern struct GUIFocus* g_Focus;
extern struct GUIEvents* g_GUIEvents;
extern struct GUIDef g_GUIDefs;
extern struct Font* g_GUIFonts;
extern struct Stack g_GUIStack;

struct Margin {
	int Top;
	int Left;
	int Right;
	int Bottom;
};

struct Area {
	int w;
	int h;
};

struct Widget {
	DECLARE_WIDGET;
};

struct Label {
	DECLARE_WIDGET;
	int (*SetText)(struct Widget*, SDL_Texture*);
	SDL_Texture* Text;
};

struct Container {
	DECLARE_CONTAINER;
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
struct Container* CreateContainer(void);
struct Table* CreateTable(void);
struct ContextItem* CreateContextItem(void);
struct GUIEvents* CreateGUIEvents(void);
struct GUIFocus* CreateGUIFocus(void);

/**
 * Constructors
 */
void ConstructWidget(struct Widget* _Widget, struct Container* _Parent,SDL_Rect* _Rect, lua_State* _State);
void ConstructLabel(struct Label* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font);
struct Button* ConstructButton(struct Button* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Texture* _Text, struct Font* _Font);
void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin);
void ConstructContextItem(struct ContextItem* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin);
void ConstructTable(struct Table* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State,
		int _Spacing, const struct Margin* _Margin, int _Columns, int _Rows, struct Font* _Font);
struct Font* CreateFont(const char* _Name, int _Size);

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child);

void WidgetSetParent(struct Container* _Parent, struct Widget* _Child);
void WidgetOnKeyUp(struct Widget* _Widget, SDL_KeyboardEvent* _Event);
void WidgetSetVisibility(struct Widget* _Widget, int _Visibility);
/**
 * Deconstructors
 */
void DestroyWidget(struct Widget* _Widget, lua_State* _State);
void DestroyLabel(struct Label* _Text, lua_State* _State);
void DestroyContainer(struct Container* _Container, lua_State* _State);
void DestroyTable(struct Table* _Table, lua_State* _State);
void DestroyFont(struct Font* _Font);
void DestroyGUIEvents(struct GUIEvents* _Events);
void DestroyFocus(struct GUIFocus* _Focus);

int ContainerOnDraw(struct Container* _Container);
void ContainerSetPosition(struct Container* _Container, const struct SDL_Point* _Point);
struct Widget* ContainerOnFocus(struct Container* _Container, const SDL_Point* _Point);
int ContainerOnUnfocus(struct Container* _Container);
int ContainerHorzFocChange(const struct Container* _Container);
struct Widget* ContainerOnClick(struct Container* _Container, const SDL_Point* _Point);
void ContainerOnDebug(const struct Container* _Container);

void VertConNewChild(struct Container* _Parent, struct Widget* _Child);
void HorzConNewChild(struct Container* _Parent, struct Widget* _Child);
void FixedConNewChild(struct Container* _Parent, struct Widget* _Child);
void ContextItemNewChild(struct Container* _Parent, struct Widget* _Child);
void ContainerShrink(struct Container* _Container);
/**
 * Returns 1 if the container can grow else returns 0.
 * _Width and _Height are how much the container needs to grow by.
 */
int WidgetGrow(struct Widget* _Widget, int _Width, int _Height);

int ContextItemOnDraw(struct ContextItem* _Container);

/*
 * Remove the child from the parent's children leaving a gap in the array.
 */
void StaticRemChild(struct Container* _Parent, struct Widget* _Child);
/*
 * Remove the child and slide all children above to fill the hole in the array.
 */
void DynamicRemChild(struct Container* _Parent, struct Widget* _Child);

int LabelOnDraw(struct Widget* _Widget);
struct Widget* LabelOnFocus(struct Widget* _Widget, const SDL_Point* _Point);
int LabelOnUnfocus(struct Widget* _Widget);
int LabelSetText(struct Widget* _Widget, SDL_Texture* _Text);

int ButtonOnDraw(struct Widget* _Widget);

void TableNewChild(struct Container* _Parent, struct Widget* _Child);
int TableHorzFocChange(const struct Container* _Container);

/*
 * Base for LuaOnKey.
 * Creates the SDL_Event and the WEvent but does not add the event function to
 * GUI.EventIds which is needed in order for the callback to function.
 */
void WidgetOnEvent(struct Widget* _Widget, int _RefId, int _Key, int _KeyState, int _KeyMod);
void WidgetSetPosition(struct Widget* _Widget, const SDL_Point* _Pos);
struct Widget* WidgetOnClick(struct Widget* _Widget, const SDL_Point* _Point);
void WidgetOnDebug(const struct Widget* _Widget);
int WidgetCheckVisibility(const struct Widget* _Widget);

struct Widget* ContextItemOnFocus(struct ContextItem* _Widget, const SDL_Point* _Point);
int ContextItemOnUnfocus(struct ContextItem* _Widget);
int ContextHorzFocChange(const struct Container* _Container);
int GetHorizontalCenter(const struct Container* _Parent, const struct Widget* _Widget);
#endif
