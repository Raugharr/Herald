/*
 * File: Video.h
 * Author: David Brotz
 */

#ifndef __VIDEO_H
#define __VIDEO_H

#include "LinkedList.h"

#include <SDL2/SDL.h>

#define SDL_CAPTION "Herald"
#define SDL_HEIGHT (1024)
#define SDL_WIDTH (768)

typedef struct lua_State lua_State;
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct _TTF_Font TTF_Font;
typedef struct SDL_Surface SDL_Surface;
struct Widget;

struct GUIFocus {
	struct Container* Parent;
	int Index;
	int Id;
};

struct WEvent {
	SDL_Event Event;
	int WidgetId;
};

struct GUIEvents {
	/* TODO: Events based on clicking a widget are not possible as the hooking widget is not stored. */
	struct WEvent* Events;
	int TblSz;
	int Size;
};

struct Font {
	TTF_Font* Font;
	char* Name;
	int Size;
	struct Font* Next;
	struct Font* Prev;
	struct LinkedList WidgetList;
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
extern int g_GUIId;
extern struct GUIFocus g_Focus;
extern struct GUIEvents g_GUIEvents;
extern struct GUIDef g_GUIDefs;
extern struct Font* g_GUIFonts;

struct Margin {
	int Top;
	int Left;
	int Right;
	int Bottom;
};

struct Widget {
	int Id;
	const struct Container* Parent;
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
	const struct Container* Parent;
	SDL_Rect Rect;
	int LuaRef;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	void (*OnDestroy)(struct Widget*);
	int (*SetText)(struct Widget*, SDL_Surface*);
	SDL_Surface* Text;
};

struct Container {
	int Id;
	const struct Container* Parent;
	SDL_Rect Rect;
	int LuaRef;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	void (*OnDestroy)(struct Widget*);
	void (*NewChild)(struct Container*, struct Widget*);
	struct Widget** Children;
	int ChildrenSz;
	int Spacing;
	struct Margin Margins;
};

struct Table {
	int Id;
	const struct Container* Parent;
	SDL_Rect Rect;
	int LuaRef;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	void (*OnDestroy)(struct Widget*);
	void (*NewChild)(struct Container*, struct Widget*);
	struct Widget** Children;
	int ChildrenSz;
	int Spacing;
	struct Margin Margins;
	int Columns;
	int Rows;
};

int VideoInit();
void VideoQuit();
int NextGUIId();
void IncrFocus(struct GUIFocus* _Focus);
void DecrFocus(struct GUIFocus* _Focus);
void Events();
void Draw();

struct TextBox* CreateTextBox();
struct Container* CreateContainer();
struct Table* CreateTable();

/**
 * Constructors
 */
void ConstructWidget(struct Widget* _Widget, struct Container* _Parent,SDL_Rect* _Rect, lua_State* _State);
void ConstructTextBox(struct TextBox* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, SDL_Surface* _Text);
void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin);
void ConstructTable(struct Table* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin, int _Columns, int _Rows);
struct Font* CreateFont(const char* _Name, int _Size);
void ContainerPosChild(struct Container* _Parent, struct Widget* _Child);
void WidgetSetParent(struct Container* _Parent, struct Widget* _Child);
/**
 * Deconstructors
 */
void DestroyWidget(struct Widget* _Widget);
void DestroyTextBox(struct TextBox* _Text);
void DestroyContainer(struct Container* _Container);
void DestroyFont(struct Font* _Font);

int ContainerOnDraw(struct Container* _Container);
int ContainerOnFocus(struct Container* _Container);
int ContainerOnUnfocus(struct Container* _Container);

void VertConNewChild(struct Container* _Parent, struct Widget* _Child);
void HorzConNewChild(struct Container* _Parent, struct Widget* _Child);

int TextBoxOnDraw(struct Widget* _Widget);
int TextBoxOnFocus(struct Widget* _Widget);
int TextBoxOnUnfocus(struct Widget* _Widget);
int WidgetSetText(struct Widget* _Widget, SDL_Surface* _Text);

int SDLEventCmp(const void* _One, const void* _Two);
int KeyEventCmp(const void* _One, const void* _Two);

SDL_Surface* ConvertSurface(SDL_Surface* _Surface);
void ChangeColor(SDL_Surface* _Surface, SDL_Color* _Prev, SDL_Color* _To);
//SDL_Surface* CreateLine(int _X1, int _Y1, int _X2, int _Y2);
#endif
