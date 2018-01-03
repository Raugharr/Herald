/*#include "Font.h"

struct IntTree g_FontHandles;
struct IntTree g_TextHandles;
FontHandle g_NextFontHdl = 1;
FT_Library g_FontLibrary;

FontHandle LoadFont(const char* _Name, uint16_t _PtSize) {*/
	/*
	 * Use FileManager to look up _Name in font folder.
	 * Use freetype to load font.
	 * Create a Font struct and fill it with data.
	 * Put Font into array that stores FontHandles.
	 */
	 /*
	 FT_Error _Error;
	 FT_CharMap _Charmap;
	 FT_Face _Face;
	 SDL_Texture* _Atlas = NULL;
	 FT_Metrics* _Metrics = NULL;
	 struct glyph _Cglyph = {0};
	 uint32_t* _Pixels = NULL; 
	 uint32_t* _Dest= NULL;
	 uint32_t* _Src = NULL;
	 int _Width = 0;
	 int _Height = 0;
	 SDL_Point _CurrPos {0, 0};

	 _Error = FT_New_Face(g_FontLibrary, _Name, 0, &_Face);
	 if(_Error != 0) {
		Log(ELOG_ERROR, "Cannot load font file %s.", _Name);
		return 0;
	 }
	 for(int i = 0; i < _Face->num_charmaps; ++i) {
		FT_CharMap _Map = _Face->charmaps[i];

		if ((charmap->platform_id == 3 && _Map->encoding_id == 1) // Windows Unicode /
			 || (_Map->platform_id == 3 && _Map->encoding_id == 0) // Windows Symbol /
			 || (_Map->platform_id == 2 && _Map->encoding_id == 1) // ISO Unicode 
			 || (_Map->platform_id == 0)) { // Apple Unicode 
				_Charmap = _Map;
				break;
			}
	 }
	 FT_Set_Charmap(_Face, _Charmap);
	 if(FT_IS_SCALABLE(_Face) != 0) {
		 _Error = FT_Set_Char_Size(Face, 0, _PtSize * 64, 0, 0);
		 if(_Error != 0) {
			 Log(ELOG_ERROR, "Cannot load font file %s. Font cannot set size.", _Name);
			return 0;
		 }
	 } else {
		 Log(ELOG_ERROR, "Cannot load font file %s. Font is not scalable.", _Name);
		return 0;
	 }
	 //FT_LoadGlyph(_Face, 0, 0);
	 for(char i = 'A'; i <= 'z'; ++i) {
		 FT_Bitmap _GlyphBit;

		_Error = FT_Load_Char(_Face, i, 0);
		if(_Error != 0) {
			Log(ELOG_ERROR, "Cannot load font file %s. Cannot load character %c.", _Name, i);
			return 0;
		}
		 _Metrics = &_Face->glyph->metrics;
		 _Cglyph.Minx = FT_FLOOR(_Metrics->horiBearingX);
		 _Cglyph.Maxx = FT_CEIL(_Metrics->horiBearingX + _Metrics->width);
		 _Cglyph.Maxy = FT_FLOOR(_Metrics->horiBearingY);
		 _Cglyph.Miny = _Cglyph.Maxy - FT_CEIL(_Metrics->height);
		 _Cglyph.Yoffset = _Font->ascent - _Cglyph->Maxy;
		 _Cglyph->Advance = FT_CEIL(_Metrics->horiAdvance);
		 _Width += _Metrics->width;
		 _Height += _Metrics->height;
		 FT_Render_Glyph(_Face->glyph, FT_RENDER_MODE_NORMAL);
		 _GlyphBit = _Face->glyph->bitmap;
	 }
	 _Pixels = calloc(_Width * _Height, sizeof(uint32_t));
	 uint8_t* _Srcp = _Src->buffer;
	 uint32_t* _Pixelsp = _Pixels;

	 for(char i = 'A'; i <= 'z'; ++i) {
		 for(int x = 0; x < _GlyphBit->rows; ++x) {
			 if(_Src->pixel_mode == FT_PIXEL_MODE_MONO) {
				 for(int y = 0; y < _GlyphBit->width; y += CHAR_BIT) {
					uint8_t _Temp = *_Srcp++;

					for(int j = 0; j < CHAR_BIT; ++j, ++_Pixelsp) {
						*_Pixelsp= (_Temp & 0xA0) >> 7
						_Temp << 1;
					}
				 }
			 }
		 }
	 }
	 _Atlas = SDL_CreateTexture(g_Renderer, SDL_PIXELFORMAT_ARGB8888, _Width, _Height);
	 SDL_UpdateTexture(g_Renderer, NULL, _Pixels, _Width * sizeof(uint32_t));
	 return g_NextFontHdl++;
}

void DestroyFont(FontHandle _Id) {
	struct Font* _Font = IntSearch(&g_FontHandles, _Id);

	if(_Font == NULL)
		return;
	IntTreeRemove(&g_FontHandles, _Id);
	free(_Font);
}

TextHandle FontCacheSolidText(FontHandle _Font, const char* _Text) {*/
	/*
	 * create a struct Character array that is equal to strlen of _Text
	 * lookup the glyph for each character and do the following,
	 * Use freetype to calculate the position of the new character via its kerning etc
	 * put store its relative position to the first character and the position of the glyph in the atlas in the ith character.
	 * Put the TextCache onto the TextCache tree and then output it's id.  
	 *//*
	return 0;
}

void RenderText(TextHandle _Text, SDL_Point* _Pos) {*/
	/*
	 * Get the TextCache that corrisponds to _Text.
	 * For each character in the TextCache put it in the batch array that corrisponds to its glyph.
	 */
	 /*
};

void DisplayText() {*/
	/*
	 * Go through every available font and do the following.
	 * Go through the font's batche arrays and render the glyph at the given position.
	 */
//}
