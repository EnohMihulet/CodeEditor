#include "font.h"

#include <print>
#include <sstream>
#include <fstream>


s16 loadTga(TgaImageRGBA& image, const std::string& path) {
	TgaHeader header{};

	std::ifstream file(path, std::ios::binary);
	if (!file) return -1;

	file.read(reinterpret_cast<char*>(&header), sizeof(header));
	if (!file) return -1;

	static_assert(sizeof(TgaHeader) == 18);

	if (header.colorMapType != 0) {
		std::println("Unsupported TGA: color-mapped");
		return -1;
	}
	if (header.imageType != 2) {
		std::println("Unsupported TGA: imageType != 2 (uncompressed truecolor)");
		return -1;
	}
	if (header.pixelDepth != 32) {
		std::println("Unsupported TGA: pixelDepth != 32");
		return -1;
	}

	if (header.idLength) {
		file.seekg(header.idLength, std::ios::cur);
		if (!file) return -1;
	}

	image.width = header.width;
	image.height = header.height;

	const u32 pixelCount = image.width * image.height;
	image.pitch = image.width * 4;

	image.pixels = new u32[pixelCount];
	file.read(reinterpret_cast<char*>(image.pixels), static_cast<std::streamsize>(pixelCount * 4));
	if (!file) return -1;

	u8* pixels = (u8*) image.pixels;
	for (u32 i = 0; i < pixelCount; ++i) {
		u8& b = pixels[i * 4 + 0];
		u8& r = pixels[i * 4 + 2];
		std::swap(b, r);
	}

	const bool topLeft = (header.imageDescriptor & 0x20) != 0;
	if (!topLeft) {
		const u32 rowPixels = image.width;
		for (u32 y = 0; y < image.height / 2; ++y) {
			u32* rowA = image.pixels + y * rowPixels;
			u32* rowB = image.pixels + (image.height - 1 - y) * rowPixels;
			for (u32 i = 0; i < rowPixels; ++i) std::swap(rowA[i], rowB[i]);
		}
	}

	return 0;
}


s16 parseKVTokens(std::unordered_map<std::string, std::string>& kv, const std::string& line) {
	std::istringstream iss(line);
	std::string token;

	iss >> token;

	while (iss >> token) {
		auto eq = token.find('=');
		if (!eq) continue;

		std::string k = token.substr(0, eq);
		std::string v = token.substr(eq + 1);

		if (!v.empty() && v.front() == '"') {
			while (v.size() < 2 || v.back() != '"') {
				std::string more;
				if (!(iss >> more)) break;
				v += " " + more;
			}
			if (v.size() >= 2 && v.front() == '"' && v.back() == '"') {
				v = v.substr(1, v.size() - 2);
			}
		}

		kv.emplace(std::move(k), std::move(v));
	}
	
	return 0;
}

s16 toI(const std::unordered_map<std::string, std::string>& kv, const char* key, s16 def) {
	auto it = kv.find(key);
	return (it == kv.end()) ? def : (s16) std::stoi(it->second);
}

s16 loadFnt(Font& font, const std::string& path) {
	std::ifstream file(path);
	if (!file) return -1;

	std::string line;
	std::unordered_map<std::string, std::string> kv;
	while (std::getline(file, line)) {
		if (line.rfind("common", 0) == 0) {
			if (parseKVTokens(kv, line) != 0) return -1;
			font.lineHeight = toI(kv, "lineHeight");
			font.base = toI(kv, "base");
			font.scaleH = toI(kv, "scaleH");
			font.scaleW = toI(kv, "scaleW");
		}
		else if (line.rfind("char", 0) == 0) {
			if (parseKVTokens(kv, line) != 0) return -1;
			Glyph g;
			g.id = toI(kv, "id");
			g.x = toI(kv, "x");
			g.y = toI(kv, "y");
			g.w = toI(kv, "width");
			g.h = toI(kv, "height");
			g.xOffset = toI(kv, "xoffset");
			g.yOffset = toI(kv, "yoffset");
			g.xAdvance = toI(kv, "xadvance");
			g.page = toI(kv, "page");
			g.channel = toI(kv, "chnl");
			font.glyphs[g.id] = std::move(g);
		}
		kv.clear();
	}

	return 0;
}


