#include "render.h"
#include "config.h"
#include "diagnostics.h"
#include <print>
#include <algorithm>

void renderCharacter(EditorState& st, char character, u32 xPos, u32 yPos, u32 color) {
	DIAG_ASSERT(character >= 32 && character <= 126, "renderCharacter non-printable character");
	Glyph g = *st.Font.getGlyph(character);

	for (u32 y = 0; y < (u32)g.h; y++) {
		for (u32 x = 0; x < (u32)g.w; x++) {
			u32 srcX = (u32)g.x + x;
			u32 srcY = (u32)g.y + y;

			if (!st.TgaFontImg.pixels[srcX + srcY * st.TgaFontImg.width]) continue;

			s32 dstX = (s32)xPos + (s32)g.xOffset + (s32)x;
			s32 dstY = (s32)yPos + (s32)g.yOffset + (s32)y;

			st.screenBuf.pixels[dstX + dstY * (s32)st.screenBuf.width] = color;
		}
	}
}

void renderNumber(EditorState& st, std::string_view number, u32 xPos, u32 yPos, u32 color) {
	for (char c : number) {
		DIAG_ASSERT(c >= '0' && c <= '9', "renderNumber received non-digit");
		Glyph g = *st.Font.getGlyph(c);

		for (u32 y = 0; y < (u32)g.h; y++) {
			for (u32 x = 0; x < (u32)g.w; x++) {
				u32 srcX = (u32)g.x + x;
				u32 srcY = (u32)g.y + y;

				if (!st.TgaFontImg.pixels[srcX + srcY * st.TgaFontImg.width]) continue;

				s32 dstX = (s32)xPos + (s32)g.xOffset + (s32)x;
				s32 dstY = (s32)yPos + (s32)g.yOffset + (s32)y;

				st.screenBuf.pixels[dstX + dstY * (s32)st.screenBuf.width] = color;
			}
		}
		xPos += (u32)g.xAdvance;
	}
}

void renderString(EditorState& st, std::string_view str, u32 xPos, u32 yPos, u32 color) {
	Glyph spaceGlyph = *st.Font.getGlyph(' ');
	for (char c : str) {
		if (c == '\t') {
			xPos += (u32)spaceGlyph.xAdvance * TAB_SIZE;
			continue;
		}

		Glyph g = *st.Font.getGlyph(c);

		for (u32 y = 0; y < (u32)g.h; y++) {
			for (u32 x = 0; x < (u32)g.w; x++) {
				u32 srcX = (u32)g.x + x;
				u32 srcY = (u32)g.y + y;

				if (!st.TgaFontImg.pixels[srcX + srcY * st.TgaFontImg.width]) continue;

				s32 dstX = (s32)xPos + (s32)g.xOffset + (s32)x;
				s32 dstY = (s32)yPos + (s32)g.yOffset + (s32)y;

				st.screenBuf.pixels[dstX + dstY * (s32)st.screenBuf.width] = color;
			}
		}
		xPos += (u32)g.xAdvance;
	}
}

void renderLineNumbers(EditorState& st) {
	for (u32 i = 0; i < st.DisplayedLineCount; i++) {
		std::string s = std::to_string(st.TopLine + i + 1);

		s32 totalW = 0;
		for (char c : s) totalW += (*st.Font.getGlyph(c)).xAdvance;

		u32 color = (st.TopLine + i == st.CurrLine) ? LIGHT_GREEN : DARK_GREEN;
		renderNumber(st, s, LPAD + LINE_NUM_LPAD - (u32)totalW, (i * LINE_HEIGHT) + TPAD + LINE_NUM_TPAD, color);
	}
}

void renderLineSeparators(EditorState& st) {
	u8* pixels = (u8*)st.screenBuf.pixels + (st.screenBuf.pitch * TPAD);

	for (u32 i = 0; i < st.DisplayedLineCount; i++) {
		for (u32 j = 0; j < st.screenBuf.width; j++) {
			u32* p = (u32*)(pixels + ((i * LINE_HEIGHT) * st.screenBuf.pitch) + (j * 4));
			*p = DARK_GREEN;
		}
	}
}

void renderSelectedLine(EditorState& st) {
	u32 yPos = TPAD + (st.CurrLine - st.TopLine) * LINE_HEIGHT;
	for (u32 y = 0; y < LINE_HEIGHT; y++) {
		for (u32 x = 0; x < st.screenBuf.width; x++) {
			st.screenBuf.pixels[x + (y + yPos) * st.screenBuf.width] = SELECTED_LINE_BG;
		}
	}
}

void renderSelectedPosition(EditorState& st, u32 color) {

	u32 xPos = LPAD + TEXT_LPAD;
	u32 yPos = TPAD;
	LineBuffer* lb = st.Text->getLineBuffer(st.TopLine);
	Glyph spaceGlyph = *st.Font.getGlyph(' ');

	for (u32 i = 0; i < st.DisplayedLineCount; i++) {
		if (lb == st.CurrLineBuffer) break;
		renderString(st, lb->text, LPAD + TEXT_LPAD, TPAD + TEXT_TPAD + i * LINE_HEIGHT, DARK_GREEN);
		if (lb->next == nullptr) break;
		lb = lb->next;
		yPos += LINE_HEIGHT;
	}

	for (u32 i = 0; i < st.CursorPos; i++) {
		char c = lb->text[i];
		if (c == '\t') {
			xPos += (u32)spaceGlyph.xAdvance * TAB_SIZE;
			continue;
		}
		Glyph g = *st.Font.getGlyph(c);
		xPos += (u32)g.xAdvance;
	}

	for (u32 x = 0; x < DEFAULT_CHAR_WIDTH; x++) {
		for (u32 y = 1; y < LINE_HEIGHT-1; y++) {
			st.screenBuf.pixels[xPos + x + (yPos + y) * (s32)st.screenBuf.width] = color;
		}
		if (st.CurrMode == InsertMode) break;
	}
}

void renderTextBuffer(EditorState& st) {
	LineBuffer* lb = st.Text->getLineBuffer(st.TopLine);

	for (u32 i = 0; i < st.DisplayedLineCount && lb; i++) {
		renderString(st, lb->text, LPAD + TEXT_LPAD, TPAD + TEXT_TPAD + i * LINE_HEIGHT, DARK_GREEN);
		lb = lb->next;
	}
}

static u32 getStringWidth(EditorState& st, std::string_view str) {
	u32 width = 0;
	for (char c : str) {
		Glyph g = *st.Font.getGlyph(c);
		width += g.xAdvance;
	}
	return width;
}

void renderBottom(EditorState& st, u32 fps) {
	u32 yStart = st.screenBuf.height - BPAD;
	for (u32 y = yStart; y < st.screenBuf.height; y++) {
		for (u32 x = 0; x < st.screenBuf.width; x++) {
			st.screenBuf.pixels[x + y * st.screenBuf.width] = GRAY_10;
		}
	}

	std::string modeLabel = "MODE: ";
	switch (st.CurrMode) {
		case NormalMode: modeLabel += "Normal"; break;
		case InsertMode: modeLabel += "Insert"; break;
		case VisualMode: modeLabel += "Visual"; break;
	}

	std::string lineLabel = "Ln " + std::to_string(st.CurrLine + 1) + ", Col " + std::to_string(st.CursorPos + 1);

	std::string fileLabel = "File: ";
	if (!st.currentFileName.empty()) fileLabel += st.currentFileName;
	else fileLabel += "untitled";

	std::string saveLabel = st.isDirty ? "Unsaved" : "Saved";
	u32 saveColor = st.isDirty ? LIGHT_RED : LIGHT_GREEN;

	std::string fpsLabel = "FPS " + std::to_string(fps);

	u32 textY = yStart + 12;
	u32 modeX = MODE_LPAD;
	u32 modeWidth = getStringWidth(st, modeLabel);
	u32 lineX = modeX + modeWidth + 32;
	u32 lineWidth = getStringWidth(st, lineLabel);

	u32 fpsWidth = getStringWidth(st, fpsLabel);
	u32 fpsX = (st.screenBuf.width > fpsWidth + MODE_LPAD) ? st.screenBuf.width - fpsWidth - MODE_LPAD : MODE_LPAD;

	u32 saveWidth = getStringWidth(st, saveLabel);
	u32 saveGap = 32;
	u32 saveX = (fpsX > saveWidth + saveGap) ? fpsX - saveWidth - saveGap : lineX + lineWidth + saveGap;
	u32 minSaveX = lineX + lineWidth + saveGap;
	saveX = std::max(saveX, minSaveX);

	u32 fileWidth = getStringWidth(st, fileLabel);
	u32 minFileX = lineX + lineWidth + 32;
	u32 maxFileX = std::max(minFileX, (saveX > fileWidth + 32) ? saveX - fileWidth - 32 : minFileX);

	s32 centerFileX = (s32)st.screenBuf.width / 2 - (s32)fileWidth / 2;
	u32 fileX = centerFileX > 0 ? (u32)centerFileX : minFileX;
	fileX = std::max(fileX, minFileX);
	fileX = std::min(fileX, maxFileX);

	renderString(st, modeLabel, modeX, textY, LIGHT_GREEN);
	renderString(st, lineLabel, lineX, textY, LIGHT_BLUE);
	renderString(st, fileLabel, fileX, textY, GRAY_90);
	renderString(st, saveLabel, saveX, textY, saveColor);
	renderString(st, fpsLabel, fpsX, textY, YELLOW);
}
