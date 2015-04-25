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
typedef struct SDL_KeyboardEvent SDL_KeyboardEvent;
typedef struct SDL_Texture SDL_Texture;
struct Widget;
struct Container;

struct Font {
	TTF_Font* Font;
	char* Name; //Replace with TTF_FontFaceStyleName.
	int Size;
	struct Font* Next;
	struct Font* Prev;
	int RefCt;
};

/*
 *
 * TODO: Remove g_Window, g_Renderer, g_VideoOk, and g_WindowTexture as globals and only allow them to be accessed by Video.c
 */
extern SDL_Window* g_Window;
extern SDL_Renderer* g_Renderer;
extern int g_VideoOk;
extern SDL_Texture* g_WindowTexture;

int VideoInit(void);
void VideoQuit(void);
int NextGUIId(void);
struct GUIFocus* ChangeFocus_Aux(struct GUIFocus* _Focus, int _Change, int _Pos);
void Events(void);
void Draw(void);

int SDLEventCmp(const void* _One, const void* _Two);
int KeyEventCmp(const void* _One, const void* _Two);

SDL_Surface* ConvertSurface(SDL_Surface* _Surface);
SDL_Texture* SurfaceToTexture(SDL_Surface* _Surface);
//void ChangeColor(SDL_Surface* _Surface, SDL_Color* _Prev, SDL_Color* _To);
int FirstFocusable(const struct Container* _Parent);
int NextFocusable(const struct Container* _Parent, int _Index, int _Pos);
int GetHorizontalCenter(const struct Container* _Parent, const struct Widget* _Widget);
SDL_Texture* CreateWorldTexture(int _Size);
//SDL_Surface* CreateLine(int _X1, int _Y1, int _X2, int _Y2);
#endif
