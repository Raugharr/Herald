#define FontHandle uint32_t
#define TextHandle uint32_t

struct Glyph {
	uint8_t Glyph;
	SDL_Point AtlasPos;
	uint8_t Minx;
	uint8_t Maxx;
	uint8_t Miny;
	uint8_t Maxy;
	uint8_t Yoffset;
	uint8_t Advance;
};

struct GlyphCache {
	struct Glyph Glyph;
	struct SDL_Point[128] Batch; //List of points to render the glyph at.
};

struct Character {
	SDL_Point AtlasPos;
	SDL_Point RenderPos;
};

struct TextCache {
	const struct Font* Font;
	struct Character* Char;
	uint16_t CharSize;
	SDL_Color Color;
};

struct Font {
	FontHandle Id;
	TTF_Font* Font;
	SDL_Texture* Atlas;
	uint16_t PtSize;
	struct GlyphCache[53] Cache;
	volatile int16_t RefCt;
};

/*
 **\return 0 if LoadFont fails otherwise returns the id of the new font.
 */
FontHandle LoadFont(const char* _Name, uint16_t _PtSize);
void DestroyFont(FontHandle _Id);
TextHandle FontCacheSolidText(FontHandle _Font, const char* _Text);
void RenderText(TextHandle _Text);
void DisplayText();
