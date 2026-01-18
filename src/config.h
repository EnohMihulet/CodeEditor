#pragma once
#include "commonTypes.h"
#include "textBuffer.h"
#include "font.h"


constexpr u32 WINDOW_WIDTH = 1920;
constexpr u32 WINDOW_HEIGHT = 1080;

constexpr u32 TPAD = 0;
constexpr u32 BPAD = 35;
constexpr u32 LPAD = 25;
constexpr u32 LINE_NUM_TPAD = 0;
constexpr u32 LINE_NUM_LPAD = 25;
constexpr u32 TEXT_TPAD = 0;
constexpr u32 TEXT_LPAD = 35;
constexpr u32 FPS_LPAD = 125;
constexpr u32 FPS_BPAD = 20;
constexpr u32 MODE_LPAD = 25;
constexpr u32 MODE_BPAD = 20;

constexpr u32 DEFAULT_CHAR_WIDTH = 12;
constexpr u32 TAB_SIZE = 4;

constexpr u32 LINE_HEIGHT = 25;
constexpr u32 SCALE_FACTOR = 1;

const char* const FNT_PATH = "media/SpaceMono_Regular_18.fnt";
const char* const TGA_PATH = "media/SpaceMono_Regular_18.tga";
const char* const FILE_PATH = "media/text.txt";

enum ModeType {
	NormalMode = 0, InsertMode = 1, VisualMode = 2
};

struct OffscreenBuffer {
	u32 width = 0, height = 0, pitch = 0;
	u32* pixels = nullptr;
};

struct EditorState {
	u32 DisplayedLineCount = 0;
	u32 MaxDisplayedLineCount = 0;
	u32 TopLine = 0;
	u32 BottomLine = 0;
	u32 CurrLine = 0;
	u32 CursorPos = 0;

	ModeType CurrMode = NormalMode;

	TextBuffer* Text = nullptr;
	LineBuffer* CurrLineBuffer = nullptr;

	u32 FontSize = 5;
	Font Font{};
	TgaImageRGBA TgaFontImg{};

	OffscreenBuffer screenBuf{};
};
