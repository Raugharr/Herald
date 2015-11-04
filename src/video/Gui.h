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

struct Container {
	DECLARE_CONTAINER;
};

struct Container* CreateContainer(void);
struct GUIEvents* CreateGUIEvents(void);
struct GUIFocus* CreateGUIFocus(void);

/**
 * Constructors
 */
void ConstructWidget(struct Widget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State);
void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin);
struct Font* CreateFont(const char* _Name, int _Size);

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child, SDL_Point* _Pos);

void WidgetSetParent(struct Container* _Parent, struct Widget* _Child);
void WidgetOnKeyUp(struct Widget* _Widget, SDL_KeyboardEvent* _Event);
void WidgetSetVisibility(struct Widget* _Widget, int _Visibility);
/**
 * Deconstructors
 */
void DestroyWidget(struct Widget* _Widget, lua_State* _State);
void DestroyContainer(struct Container* _Container, lua_State* _State);
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

/*
 * Remove the child from the parent's children leaving a gap in the array.
 */
void StaticRemChild(struct Container* _Parent, struct Widget* _Child);
/*
 * Remove the child and slide all children above to fill the hole in the array.
 */
void DynamicRemChild(struct Container* _Parent, struct Widget* _Child);

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

int GetHorizontalCenter(const struct Container* _Parent, const struct Widget* _Widget);
struct Widget* WidgetOnFocus(struct Widget* _Widget, const SDL_Point* _Point);
int WidgetOnUnfocus(struct Widget* _Widget);
#endif
