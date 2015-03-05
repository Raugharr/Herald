/*
 * File: Video.h
 * Author: David Brotz
 */

#ifndef __VIDEO_H
#define __VIDEO_H

#include "Stack.h"

#include <SDL2/SDL.h>

#define SDL_CAPTION "Herald"
#define SDL_HEIGHT (768)
#define SDL_WIDTH (1024)
#define ChangeFocus(_Focus, _Change) ((_Change < 0) ? (ChangeFocus_Aux(_Focus, -_Change, -1)) : (ChangeFocus_Aux(_Focus, _Change, 1)))

typedef struct lua_State lua_State;
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct _TTF_Font TTF_Font;
typedef struct SDL_Surface SDL_Surface;
struct Widget;

struct GUIFocus {
	const struct Container* Parent;
	int Index;
	int Id;
	struct GUIFocus* Prev;
};

struct WEvent {
	SDL_Event Event;
	int WidgetId;
	int RefId;
};

struct GUIEvents {
	/* TODO: Events based on clicking a widget are not possible as the hooking widget is not stored. */
	struct WEvent* Events;
	int TblSz;
	int Size;
};

struct Font {
	TTF_Font* Font;
	char* Name; //Replace with TTF_FontFaceStyleName.
	int Size;
	struct Font* Next;
	struct Font* Prev;
	int RefCt;
};

struct GUIDef {
	struct Font* Font;
	SDL_Color FontFocus;
	SDL_Color FontUnfocus;
	SDL_Color Background;
};

extern SDL_Window* g_Window;
extern SDL_Renderer* g_Renderer;
extern int g_GUIOk;
extern int g_GUIMenuChange;
extern int g_GUIId;
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
	int Id;
	struct Container* Parent;
	SDL_Rect Rect;
	int LuaRef;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	void (*OnDestroy)(struct Widget*);
};

struct TextBox {
	int Id;
	struct Container* Parent;
	SDL_Rect Rect;
	int LuaRef;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	void (*OnDestroy)(struct Widget*);
	int (*SetText)(struct Widget*, SDL_Surface*);
	SDL_Surface* Text;
	struct Font* Font;
};

struct Container {
	int Id;
	struct Container* Parent;
	SDL_Rect Rect;
	int LuaRef;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	void (*OnDestroy)(struct Widget*);
	void (*NewChild)(struct Container*, struct Widget*);
	void (*RemChild)(struct Container*, struct Widget*);
	struct Widget** Children;
	int ChildrenSz;
	int ChildCt;
	int Spacing;
	int FocusChange;
	struct Margin Margins;
};

struct Table {
	int Id;
	struct Container* Parent;
	SDL_Rect Rect;
	int LuaRef;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	void (*OnDestroy)(struct Widget*);
	void (*NewChild)(struct Container*, struct Widget*);
	void (*RemChild)(struct Container*, struct Widget*);
	struct Widget** Children;
	int ChildrenSz;
	int ChildCt;
	int Spacing;
	int FocusChange;
	struct Margin Margins;
	int Rows;
	int Columns;
	struct Area CellMax; /* max area of a cell. */
	struct Font* Font;
};

struct ContextItem {
	int Id;
	struct Container* Parent;
	SDL_Rect Rect;
	int LuaRef;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	void (*OnDestroy)(struct Widget*);
	void (*NewChild)(struct Container*, struct Widget*);
	void (*RemChild)(struct Container*, struct Widget*);
	struct Widget** Children;
	int ChildrenSz;
	int ChildCt;
	int Spacing;
	int FocusChange;
	struct Margin Margins;
	int ShowContexts;
};

int VideoInit(void);
void VideoQuit(void);
int NextGUIId(void);
struct GUIFocus* ChangeFocus_Aux(struct GUIFocus* _Focus, int _Change, int _Pos);
void Events(void);
void Draw(void);

struct TextBox* CreateTextBox(void);
struct Container* CreateContainer(void);
struct Table* CreateTable(void);
struct ContextItem* CreateContextItem(void);
struct GUIEvents* CreateGUIEvents(void);
struct GUIFocus* CreateGUIFocus(void);

/**
 * Constructors
 */
void ConstructWidget(struct Widget* _Widget, struct Container* _Parent,SDL_Rect* _Rect, lua_State* _State);
void ConstructTextBox(struct TextBox* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Surface* _Text, struct Font* _Font);
void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin);
void ConstructContextItem(struct ContextItem* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin);
void ConstructTable(struct Table* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State,
		int _Spacing, const struct Margin* _Margin, int _Columns, int _Rows, struct Font* _Font);
struct Font* CreateFont(const char* _Name, int _Size);

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child);

void WidgetSetParent(struct Container* _Parent, struct Widget* _Child);
/**
 * Deconstructors
 */
void DestroyWidget(struct Widget* _Widget);
void DestroyTextBox(struct TextBox* _Text);
void DestroyContainer(struct Container* _Container);
void DestroyTable(struct Table* _Table);
void DestroyFont(struct Font* _Font);
void DestroyGUIEvents(struct GUIEvents* _Events);
void DestroyFocus(struct GUIFocus* _Focus);

int ContainerOnDraw(struct Container* _Container);
int ContainerOnFocus(struct Container* _Container);
int ContainerOnUnfocus(struct Container* _Container);

void VertConNewChild(struct Container* _Parent, struct Widget* _Child);
void HorzConNewChild(struct Container* _Parent, struct Widget* _Child);
void ContextItemNewChild(struct Container* _Parent, struct Widget* _Child);

int ContextItemOnDraw(struct ContextItem* _Container);

/*
 * Remove the child from the parent's children leaving a gap in the array.
 */
void StaticRemChild(struct Container* _Parent, struct Widget* _Child);
/*
 * Remove the child and slide all children above to fill the hole in the array.
 */
void DynamicRemChild(struct Container* _Parent, struct Widget* _Child);

int TextBoxOnDraw(struct Widget* _Widget);
int TextBoxOnFocus(struct Widget* _Widget);
int TextBoxOnUnfocus(struct Widget* _Widget);
int WidgetSetText(struct Widget* _Widget, SDL_Surface* _Text);

void TableNewChild(struct Container* _Parent, struct Widget* _Child);

int SDLEventCmp(const void* _One, const void* _Two);
int KeyEventCmp(const void* _One, const void* _Two);
/*
 * Base for LuaOnKey.
 * Creates the SDL_Event and the WEvent but does not add the event function to
 * GUI.EventIds which is needed in order for the callback to function.
 */
void WidgetOnEvent(struct Widget* _Widget, int _RefId, int _Key, int _KeyState, int _KeyMod);

void ContextItemOnEnter(struct ContextItem* _Widget);

SDL_Surface* ConvertSurface(SDL_Surface* _Surface);
void ChangeColor(SDL_Surface* _Surface, SDL_Color* _Prev, SDL_Color* _To);
int FirstFocusable(const struct Container* _Parent);
int GetHorizontalCenter(const struct Container* _Parent, const struct Widget* _Widget);
//SDL_Surface* CreateLine(int _X1, int _Y1, int _X2, int _Y2);
#endif
