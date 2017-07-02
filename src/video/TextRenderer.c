/**
 * Author David Brotz
 * File: TextRender.c
 */
#include "TextRenderer.h"

#include "Video.h"

#include "../sys/Log.h"
#include "../sys/Array.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <freetype2/ftmodapi.h>
#include <freetype2/ftglyph.h>
#include <freetype2/ftstroke.h>
#include <freetype/ftbitmap.h>

#define ASCII_OFFSET (0x20)
#define ASCII_END (0xFF)
#define TextBufferSz (0x7E - ASCII_OFFSET)

#define FT_FLOOR(X) ((X & -64) / 64)
#define FT_CEIL(X)  (((X + 63) & -64) / 64)

struct Array g_FontList = {NULL, 0, 0};
static FT_Library g_FtLibrary = NULL;

bool InitTextRenderer() {
	FT_Error Error = NULL;

	if(g_FtLibrary != NULL) {
		Log(ELOG_WARNING, "Text renderer is already initialized.");
		return true;
	}
	if((Error = FT_Init_FreeType(&g_FtLibrary)) != 0) {
		Log(ELOG_ERROR, "Cannot init text renderer, error %X.", Error);
		return false;
	}
	return true;
}

void QuitTextRenderer() {
	FT_Error Error = NULL;

	if((Error = FT_Done_Library(g_FtLibrary)) != 0) {
		Log(ELOG_ERROR, "Cannot quit text renderer, error %X.", Error);
	}
}

static unsigned long RWread(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count) {
    SDL_RWops *src;

    src = (SDL_RWops *)stream->descriptor.pointer;
    SDL_RWseek( src, (int)offset, RW_SEEK_SET );
    if ( count == 0 ) {
        return 0;
    }
    return (unsigned long)SDL_RWread( src, buffer, 1, (int)count );
}

struct Font* OpenFont(SDL_RWops* File, uint8_t FontSz) {
	int64_t FilePos = 0;
	struct Font* Font = NULL;
	struct Cglyph* Glyph = NULL;
	SDL_Surface* TextBuff = NULL;
	SDL_Palette* Palette = NULL;
	FT_Bitmap* Bitmap = NULL;
	FT_Glyph_Metrics* Metrics = NULL;
	FT_GlyphSlot GlyphSlot = NULL;
	FT_Open_Args Args = {0};
	FT_Stream Stream = NULL;
	FT_Fixed Scale;
	FT_Face Face = NULL;
	//FT_Stroker Stroker = NULL; 
	FT_Error Error;
	uint8_t* Dst = NULL;
	uint8_t* Src = NULL;
	uint8_t* DstEnd = NULL;

	if(File == NULL) return NULL;
	FilePos = SDL_RWtell(File);
	if(FilePos < 0) return NULL;
	Font = malloc(sizeof(struct Font));

	Stream = malloc(sizeof(*Stream));
	Stream->read = RWread;
	Stream->descriptor.pointer = File;
	Stream->pos = (unsigned long)FilePos;
	Stream->size = (unsigned long)(SDL_RWsize(File) - FilePos);
	Args.flags = FT_OPEN_STREAM;
	Args.stream = Stream;

    if(FT_Open_Face(g_FtLibrary, &Args, 0, &Font->Face) != 0) {
		Log(ELOG_ERROR, "Cnnot load font, cannot open face.");
		free(Font);
		return NULL;
	}
	Face = Font->Face;
	for (int i = 0; i < Face->num_charmaps; i++) {
		FT_CharMap Charmap = Face->charmaps[i];

		if ((Charmap->platform_id == 3 && Charmap->encoding_id == 1) /* Windows Unicode */
			|| (Charmap->platform_id == 3 && Charmap->encoding_id == 0) /* Windows Symbol */
			|| (Charmap->platform_id == 2 && Charmap->encoding_id == 1) /* ISO Unicode */
			|| (Charmap->platform_id == 0)) { /* Apple Unicode */
				FT_Set_Charmap(Face, Charmap);
				break;
			}
    }
	if(FT_IS_SCALABLE(Face) == 0) {
		Log(ELOG_ERROR, "Cannot load font, font not scalable.");
		free(Font);
		return NULL;
	}
	FT_Set_Char_Size(Face, 0, FontSz * 64, 0, 0);
	Scale = Face->size->metrics.y_scale;
	Font->PtSize = FontSz;
	Font->Ascent  = FT_CEIL(FT_MulFix(Face->ascender, Scale));
	Font->Descent = FT_CEIL(FT_MulFix(Face->descender, Scale));
	Font->Height = Font->Ascent - Font->Descent + 1;
	Font->LineSpace = FT_CEIL(Face->size->metrics.height);
	for(int i = ASCII_OFFSET; i < ASCII_END; ++i) {
		if((Error = FT_Load_Char(Face, i, FT_LOAD_DEFAULT)) != 0) return NULL;
		//if((Error = FT_Get_Glyph(Face->glyph, &FGlyph)) != 0) return NULL;
		GlyphSlot = Face->glyph;
		if((Error = FT_Render_Glyph(GlyphSlot, FT_RENDER_MODE_MONO)) != 0) return NULL;	
		/*
		if((Error = FT_Get_Glyph(GlyphSlot, &FGlyph)) != 0)
			continue;

		if((Error = FT_Stroker_New(g_FtLibrary, &Stroker)) != 0) {
			Log(ELOG_ERROR, "Cannot create font, stroker cannot be created, error: %X", Error);
			goto error;
		}
		FT_Stroker_Set(Stroker, 1 * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
		if((Error = FT_Glyph_Stroke(&FGlyph, Stroker, 1)) != 0) goto error;
		FT_Stroker_Done(Stroker);
		
		if((Error = FT_Glyph_To_Bitmap(&FGlyph, FT_RENDER_MODE_NORMAL, 0, 1)) != 0) {
			Log(ELOG_ERROR, "Cannot create font, glyph cannot be converted to bitmap, error: %X.", Error);
			goto error;
		Bitmap = &Glyph->Bitmap;
		}*/
		Bitmap = &GlyphSlot->bitmap;
		Metrics = &Face->glyph->metrics;
		Glyph = &Font->Cglyph[i];
		Glyph->Minx = FT_FLOOR(Metrics->horiBearingX);
		Glyph->Maxx = FT_CEIL(Metrics->horiBearingX + Metrics->width);
		Glyph->Maxy = FT_FLOOR(Metrics->horiBearingY);
		Glyph->Miny = Glyph->Maxy - FT_CEIL(Metrics->height);
		Glyph->Yoffset = Font->Ascent - Glyph->Maxy;
		Glyph->Advance = FT_CEIL(Metrics->horiAdvance);
		Glyph->Width = FT_FLOOR(Metrics->width);
		Glyph->Height = FT_FLOOR(Metrics->height);

		TextBuff = SDL_CreateRGBSurface(0, Bitmap->width, Bitmap->rows, 8, 0, 0, 0, 0);
		SDL_memset(TextBuff->pixels, 0, TextBuff->pitch * TextBuff->h);
		if(TextBuff == NULL) {
			Log(ELOG_ERROR, "Cannot load font: glyph surface cannot be created.");
			free(Font);
			return NULL;
		}
		DstEnd = (uint8_t*)TextBuff->pixels + TextBuff->pitch * TextBuff->h;
		if(Bitmap->pixel_mode == FT_PIXEL_MODE_MONO) {
			Palette = TextBuff->format->palette;
			Palette->colors[0].r = 0;
			Palette->colors[0].g = 0;
			Palette->colors[0].b = 0;
			Palette->colors[1].r = 255;
			Palette->colors[1].g = 255;
			Palette->colors[1].b = 255;
			SDL_SetColorKey(TextBuff, SDL_TRUE, 0);
			for(int Row = 0; Row < Bitmap->rows; ++Row) {
				Dst = (Uint8*) TextBuff->pixels + Row * TextBuff->pitch;
				Src = Bitmap->buffer + Row * Bitmap->pitch;

				for(uint8_t Col = 0; Col < Bitmap->width /*&& (Dst + 8) < DstEnd*/; Col += 8) {
					uint8_t Bit = *Src++;
					if(Dst >= DstEnd) break;
					*Dst++ = (Bit & 0x80) >> 7;
					if(Dst >= DstEnd) break;
					*Dst++ = (Bit & 0x40) >> 6;
					if(Dst >= DstEnd) break;
					*Dst++ = (Bit & 0x20) >> 5;
					if(Dst >= DstEnd) break;
					*Dst++ = (Bit & 0x10) >> 4;
					if(Dst >= DstEnd) break;
					*Dst++ = (Bit & 0x08) >> 3;
					if(Dst >= DstEnd) break;
					*Dst++ = (Bit & 0x04) >> 2;
					if(Dst >= DstEnd) break;
					*Dst++ = (Bit & 0x02) >> 1;
					if(Dst >= DstEnd) break;
					*Dst++ = (Bit & 0x01);
				}
			}
		} else if(Bitmap->pixel_mode == FT_PIXEL_MODE_GRAY) {
				Palette = TextBuff->format->palette;
				for(int i = 0; i < 256; ++i) {
					Palette->colors[i].r = i;
					Palette->colors[i].g = i;
					Palette->colors[i].b = i;
				}
				SDL_SetColorKey(TextBuff, SDL_TRUE, 0);
				for(int Row = 0; Row < Bitmap->rows; ++Row) {
				Dst = (Uint8*) TextBuff->pixels + Row * TextBuff->pitch;
				Src = Bitmap->buffer + Row * Bitmap->pitch;

				for(uint8_t Col = 0; Col < Bitmap->width && Dst < DstEnd; ++Col) {
					*Dst++ = *Src++;
				}
			}
		}
		Glyph->Surface = TextBuff;
		if((Glyph->Texture = SDL_CreateTextureFromSurface(g_Renderer, TextBuff)) == NULL) {
			Log(ELOG_ERROR, "Cannot create texture for glyph '%c': %s", i, SDL_GetError());
		}
		TextBuff = NULL;
	}
	//FT_Done_Glyph(FGlyph);
	return Font;
	free(Font);
	free(Stream);
	return NULL;
}

void CtorTextStream(struct TextStream* Stream, const struct Font* Font) {
	Stream->Font = Font;
	Stream->StreamSz = 16;
	Stream->StreamCt = 0;
	Stream->Pos = -1;
	Stream->Stream = calloc(Stream->StreamSz, sizeof(char));
	Stream->Kerning = calloc(Stream->StreamSz, sizeof(int8_t));
}

void DtorTextStream(struct TextStream* Stream) {
	free(Stream->Stream);
	free(Stream->Kerning);
}

void TextStreamPush(struct TextStream* Stream, char c) {
	FT_Vector Kerning;
	FT_Error Error;

	Assert(c >= 0);
	if(Stream->StreamSz >= UINT_MAX) return;
	if(Stream->StreamCt >= Stream->StreamSz) {
		uint16_t NewSz = Stream->StreamSz * 2;
		char* NewStream = calloc(NewSz, sizeof(char));
		int8_t* NewKerning = calloc(NewSz, sizeof(int8_t));

		memcpy(NewStream, Stream->Stream, Stream->StreamCt);
		memcpy(NewKerning, Stream->Kerning, Stream->StreamCt);
		free(Stream->Stream);
		free(Stream->Kerning);
		Stream->Stream = NewStream;
		Stream->Kerning = NewKerning;
		Stream->StreamSz = NewSz;
	}
	if((Error = FT_Get_Kerning(Stream->Font->Face, Stream->Stream[Stream->Pos], c, FT_KERNING_DEFAULT, &Kerning)) != 0) {
		Log(ELOG_ERROR, "TextStreamPush error: %i.", Error);
		return;
	}
	Stream->Stream[Stream->Pos + 1] = c;
	Stream->Kerning[Stream->Pos + 1] = Kerning.x >> 6;
	++Stream->StreamCt;
	++Stream->Pos;
}

void TextStreamPop(struct TextStream* Stream) {
	int Size = 0;

	if(Stream->StreamCt <= 0) return;
	Size = Stream->StreamCt - Stream->Pos;

	memcpy(Stream->Stream + Stream->Pos, Stream->Stream + Stream->Pos + 1, Size);
	memcpy(Stream->Kerning + Stream->Pos, Stream->Kerning + Stream->Pos + 1, Size);
	--Stream->StreamCt;
	--Stream->Pos;
}

void TextStreamSet(struct TextStream* Stream, const char* Text) {
	const char* Idx = NULL;
	size_t TextLen = strlen(Text) + 1;
	uint32_t Size = 1;
	FT_Vector Kerning;

	//Initialize text buffer.
	Stream->Stream = calloc(TextLen, sizeof(char));
	Stream->Kerning = calloc(TextLen - 1, sizeof(uint8_t));
	Stream->StreamSz = TextLen;
	strcpy(Stream->Stream, Text);
	Idx = Stream->Stream;
	//Fetch the text surface for each character and then get the kerning between it and the previous character.
	++Idx;
	while(Idx != NULL && Size < Stream->StreamSz) {
		FT_Get_Kerning(Stream->Font->Face, (*(Idx - 1)), (*Idx), FT_KERNING_DEFAULT, &Kerning);
		Stream->Kerning[Size] = Kerning.x >> 6;
		++Size;
		++Idx;
	}
}

int TextStreamRender(const struct TextStream* Stream, SDL_Renderer* Renderer, const SDL_Rect* Rect) {
	SDL_Rect Pen;
	int Error = 0;

	Pen.x = Rect->x;
	Pen.y = Rect->y;
	for(int i = 0; i < Stream->StreamCt; ++i) {
		const struct Cglyph* Glyph = &Stream->Font->Cglyph[(int)Stream->Stream[i]];

		Pen.x += Glyph->Minx;
		Pen.y = Rect->y + Glyph->Yoffset;
		Pen.w = Glyph->Width;
		Pen.h = Glyph->Height;
		//if(Pen.x + Glyph->Width > Rect->w) break;
		if(Stream->Stream[i] != ' ') {
			if((Error = SDL_RenderCopy(Renderer, Glyph->Texture, NULL, &Pen)) != 0) return Error;
		}
		Pen.x += Glyph->Advance + Stream->Kerning[i];
	}
	return 0;
}

void TextStreamClear(struct TextStream* Stream) {
	Stream->StreamCt = 0;
	Stream->Pos = -1;
}

SDL_Texture* CreateSolidText(const struct Font* Font, const char* Text, const SDL_Color* Color) {
	uint32_t Width = 0;
	uint32_t Height = 0;
	uint8_t* DstEnd = NULL;
	uint8_t* Dst = NULL;
	uint8_t* Src = NULL;
	int XOff = 0;
	SDL_Surface* Buffer = NULL;
	SDL_Surface* Surface = NULL;
	SDL_Texture* Texture = NULL;
	SDL_Palette* Palette = NULL;
	FT_Vector Kerning;

	for(int i = 0; Text[i] != '\0'; ++i) {
		const struct Cglyph* Glyph = NULL;
		
		Glyph = &Font->Cglyph[(uint8_t) Text[i]];
		FT_Get_Kerning(Font->Face, Text[i], Text[i + 1], FT_KERNING_DEFAULT, &Kerning);
		Width += Glyph->Advance + (Kerning.x >> 6);
	}
	Height = Font->Height;
	Surface = SDL_CreateRGBSurface(0, Width, Height, 8, 0, 0, 0, 0);
	Palette = Surface->format->palette;
	Palette->colors[0].r = 255 - Color->r;
	Palette->colors[0].g = 255 - Color->g;
	Palette->colors[0].b = 255 - Color->b;
	Palette->colors[1].r = Color->r;
	Palette->colors[1].g = Color->g;
	Palette->colors[1].b = Color->b;
	SDL_SetColorKey(Surface, SDL_TRUE, 0);
	DstEnd = (uint8_t*)Surface->pixels + Surface->pitch * Surface->h;
	for(int i = 0; Text[i] != '\0'; ++i) {
		const struct Cglyph* Glyph = &Font->Cglyph[(uint8_t) Text[i]];

		Buffer = Glyph->Surface;
		for(int Row = 0; Row < Buffer->h; ++Row) {
			//if(Row + Glyph->Yoffset < 0) continue;
			//if(Row + Glyph->Yoffset >= Surface->w) continue;
			Dst = (Uint8*) Surface->pixels + (Row + Glyph->Yoffset) * Surface->pitch + (XOff + Glyph->Minx);
			Src = Buffer->pixels + Row * Buffer->pitch;

			for(uint8_t Col = 0; Col < Buffer->w && Dst < DstEnd; ++Col) {
				*Dst++ = *Src++;
			}
		}
		FT_Get_Kerning(Font->Face, Text[i], Text[i + 1], FT_KERNING_DEFAULT, &Kerning);
		XOff += Glyph->Advance + (Kerning.x >> 6);
	}
	Texture = SurfaceToTexture(ConvertSurface(Surface));
	return Texture;
}

int FontSearchCB(const void* One, const void* Two) {
	const struct Font* FOne = One;
	const struct Font* FTwo = Two;
	int Result = 0;

	Result = strcmp(FOne->Name, FTwo->Name);
	if(Result != 0) return Result;
	return FOne->PtSize - FTwo->PtSize;
}

struct Font* FindFont(const char* Name, int PtSize) {
	struct Font Font = {Name, PtSize, 0};

	return BinarySearch(&Font, g_FontList.Table, g_FontList.Size, FontSearchCB);
}

struct Font* CreateFont(const char* _Name, int _Size) {
	struct Font* Font = NULL;
	SDL_RWops *RW = SDL_RWFromFile(_Name, "rb");

	if((Font = FindFont(_Name, _Size)) != NULL) return Font;
	Font = OpenFont(RW, _Size);
	if((Font->Font = TTF_OpenFont(_Name, _Size)) == NULL) {
		Log(ELOG_ERROR, TTF_GetError());
		free(Font);
		return NULL;
	}
	Font->Name = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy((char*)Font->Name, _Name);
	Font->RefCt = 1;
	ArrayInsertSort_S(&g_FontList, Font, FontSearchCB);
	return Font;
}

void DestroyFont(struct Font* Font) {
	if(--Font->RefCt > 0)
		return;
	TTF_CloseFont(Font->Font);
	//FT_DoneFace(Font->Face);
	free((char*)Font->Name);
	free(Font);
}
