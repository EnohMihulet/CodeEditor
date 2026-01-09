#include "commonTypes.h"
#include <cassert>
#include <string>
#include <unordered_map>


struct Glyph {
	s16 id = 0;
	u16 x = 0, y = 0;
	u16 w = 0, h = 0;
	s16 xOffset = 0, yOffset = 0;
	u16 xAdvance = 0;
	u16 page = 0;
	u16 channel = 0;
};

struct Font {
	u16 lineHeight = 0;
	u16 base = 0;
	u16 scaleW = 0;
	u16 scaleH = 0;

	Glyph glyphs[128];

	const Glyph* getGlyph(char c) {
		assert(0 <= c && c <= 127);
		return &glyphs[(u8)c];
	}
};

#pragma pack(push, 1)
struct TgaHeader {
	u8  idLength;
	u8  colorMapType;
	u8  imageType;
	u16 cmFirstEntry;
	u16 cmLength;
	u8  cmEntrySize;
	u16 xOrigin;
	u16 yOrigin;
	u16 width;
	u16 height;
	u8  pixelDepth;
	u8  imageDescriptor;
};
#pragma pack(pop)

struct TgaImageRGBA {
	u32 width = 0; u32 height = 0;
	u32 pitch;
	u32* pixels;
};

s16 loadTga(TgaImageRGBA& image, const std::string& path);

s16 parseKVTokens(std::unordered_map<std::string, std::string>& kv, const std::string& line);

s16 toI(const std::unordered_map<std::string, std::string>& kv, const char* key, s16 def=1);

s16 loadFnt(Font& font, const std::string& path);


