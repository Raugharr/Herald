/*
 * File: Video.h
 * Author: David Brotz
 */

#ifndef __VIDEO_H
#define __VIDEO_H

#include "../sys/Stack.h"

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
struct KeyMouseState;

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
uint32_t NextGUIId(void);
void Draw(void);
int VideoEvents(const struct KeyMouseState* _State);
int KeyEventCmp(const struct KeyMouseState* _One, const struct KeyMouseState* _Two);

SDL_Surface* ConvertSurface(SDL_Surface* _Surface);
SDL_Texture* SurfaceToTexture(SDL_Surface* _Surface);
//void ChangeColor(SDL_Surface* _Surface, SDL_Color* _Prev, SDL_Color* _To);
void FocusableWidgetNull(void);
const struct Widget* GetFocusableWidget(void);

void* DownscaleImage(void* _Image, int _Width, int _Height, int _ScaleArea);
void NewZoneColor(SDL_Color* _Color);
#endif
