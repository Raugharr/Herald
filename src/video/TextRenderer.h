/**
 * Author David Brotz
 * File: TextRender.h
 */

#ifndef __TEXTRENDER_H
#define __TEXTRENDER_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Rect SDL_Rect;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_RWops SDL_RWops;
typedef struct SDL_Color SDL_Color;
typedef struct _TTF_Font TTF_Font;

struct Cglyph {
    int8_t Minx;
    int8_t Maxx;
    int8_t Miny;
    int8_t Maxy;
    int16_t Yoffset;
    int8_t Advance;
	int8_t Width;
	int8_t Height;
	SDL_Surface* Surface;
	SDL_Texture* Texture;
};

struct TextBuffer {
	char Buffer[64];
	uint8_t* Kerning;
	struct TextBuffer* Next;
	uint8_t BuffSize;
};

struct TextStream {
	char* Stream;
	int8_t* Kerning;//Buffer of kerning values from char i and i + 1.
	const struct Font* Font;
	uint16_t StreamSz;	
	uint16_t StreamCt;
	int16_t Pos;//Position in stream to add/remove characters from.
};

struct Font {
	const char* Name; //Replace with TTF_FontFaceStyleName.
	uint8_t PtSize;
	uint8_t Height;
	int8_t Ascent;
	int8_t Descent;
	int16_t LineSpace;
	TTF_Font* Font;
	FT_Face Face;
	volatile int16_t RefCt;
	struct Cglyph Cglyph[257];
};

bool InitTextRenderer();
void QuitTextRenderer();
struct Font* OpenFont(SDL_RWops* File, uint8_t FontSz);

void CtorTextStream(struct TextStream* Buffer, const struct Font* Font);
void DtorTextStream(struct TextStream* Stream);
/*
 * Adds the character c to the stream at the stream's current position and then
 * adds 1 to the current position.
 */
void TextStreamPush(struct TextStream* Stream, char c);
/*
 * Removes the character at the stream's current position, then moves the
 * position back by 1.
 */
void TextStreamPop(struct TextStream* Stream);
/*
 * Renders a text stream to Pos.
 */
int TextStreamRender(const struct TextStream* Stream, SDL_Renderer* Renderer, const SDL_Rect* Rect);
/*
 * Generate a static texture that contains the characters in Text.
 */
void TextStreamClear(struct TextStream* Stream);
SDL_Texture* CreateSolidText(const struct Font* Font, const char* Text, const SDL_Color* Color);
void TextStreamSet(struct TextStream* Buffer, const char* Text);

#endif

