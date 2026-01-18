#include "render.h"
#include "config.h"
#include "diagnostics.h"
#include <print>

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

void renderCurrentMode(EditorState& st) {
	std::string_view mode = "Normal";
	switch (st.CurrMode) {
		case NormalMode: mode = "Normal"; break;
		case InsertMode: mode = "Insert"; break;
		case VisualMode: mode = "Visual"; break;
	}
	renderString(st, mode, MODE_LPAD, st.screenBuf.height - MODE_BPAD, RED);
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

void renderFPS(EditorState& st, u32 fps) {
	renderNumber(st, std::to_string(fps), FPS_LPAD, st.screenBuf.height - FPS_BPAD, RED);
}

void renderBottom(EditorState& st) {

}
