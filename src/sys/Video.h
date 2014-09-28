/*
 * File: Video.h
 * Author: David Brotz
 */

#ifndef __VIDEO_H
#define __VIDEO_H

#include <SDL2/SDL.h>

#define SDL_CAPTION "Herald"
#define SDL_HEIGHT (1024)
#define SDL_WIDTH (768)

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct _TTF_Font TTF_Font;
typedef struct SDL_Surface SDL_Surface;
struct Widget;

struct GUIFocus {
	struct Widget* Parent;
	int Index;
};

struct GUIEvents {
	SDL_Event* Events;
	int TblSz;
	int Size;
};

struct GUIDef {
	TTF_Font* Font;
	SDL_Color FontFocus;
	SDL_Color FontUnfocus;
};

extern SDL_Window* g_Window;
extern SDL_Renderer* g_Renderer;
extern int g_GUIOk;
extern int g_GUIId;
extern struct GUIFocus g_Focus;
extern struct GUIEvents g_GUIEvents;
extern struct GUIDef g_GUIDefs;

struct Margin {
	int Top;
	int Left;
	int Right;
	int Bottom;
};

struct Widget {
	int Id;
	SDL_Rect Rect;
	const struct Widget* Parent;
	struct Widget** Children;
	int ChildrenSz;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
};

struct TextBox {
	int Id;
	SDL_Rect Rect;
	const struct Widget* Parent;
	struct Widget** Children;
	int ChildrenSz;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	int (*SetText)(struct Widget*, SDL_Surface*);
	SDL_Surface* Text;
};

struct Container {
	int Id;
	SDL_Rect Rect;
	struct Widget* Parent;
	struct Widget** Children;
	int ChildrenSz;
	int CanFocus;
	int (*OnDraw)(struct Widget*);
	int (*OnFocus)(struct Widget*);
	int (*OnUnfocus)(struct Widget*);
	int Spacing;
	struct Margin Margins;
};

int VideoInit();
void VideoQuit();
int NextGUIId();
void IncrFocus(struct GUIFocus* _Focus);
void DecrFocus(struct GUIFocus* _Focus);
void Events();
void Draw();
/**
 * Constructors
 */

void ConstructWidget(struct Widget* _Widget, const struct Widget* _Parent,SDL_Rect* _Rect);
struct TextBox* CreateText(const struct Widget* _Parent, SDL_Rect* _Rect, SDL_Surface* _Text);
struct Container* CreateContainer(const struct Widget* _Parent, SDL_Rect* _Rect, int _Spacing, const struct Margin* _Margin);

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child);
void WidgetSetParent(struct Widget* _Parent, struct Widget* _Child);
/**
 * Deconstructors
 */

void DestroyText(struct TextBox* _Text);
void DestroyContainer(struct Container* _Container);

int WidgetOnDraw(struct Widget* _Widget);
int WidgetOnFocus(struct Widget* _Widget);
int WidgetOnUnfocus(struct Widget* _Widget);

int TextBoxOnDraw(struct Widget* _Widget);
int TextBoxOnFocus(struct Widget* _Widget);
int TextBoxOnUnfocus(struct Widget* _Widget);
int WidgetSetText(struct Widget* _Widget, SDL_Surface* _Text);

int SDLEventCmp(const void* _One, const void* _Two);
int KeyEventCmp(const void* _One, const void* _Two);

void ChangeColor(SDL_Surface* _Surface, SDL_Color* _Prev, SDL_Color* _To);
#endif
