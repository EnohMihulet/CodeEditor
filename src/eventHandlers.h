#pragma once
#include <cassert>
#include <cstring>

#include <SDL.h>

#include "config.h"

enum InsertionPosition {
	InsertAtCursor = 0,
	InsertAfterCursor = 1,
};


SDL_Texture* resizeWindow(EditorState& st, u32 w, u32 h, SDL_Renderer* ren);

void moveDownOneLine(EditorState& st);

void moveUpOneLine(EditorState& st);

void jumpToLine(EditorState& st, u32 line);

void jumpToStartOfLine(EditorState& st);

void jumpToFirstNonWhitespace(EditorState& st);

void jumpToEndOfLine(EditorState& st);

void jumpToTopOfWindow(EditorState& st);

void jumpToMiddleOfWindow(EditorState& st);

void jumpToBottomOfWindow(EditorState& st);

void jumpToNextWord(EditorState& st);

void jumpToPrevWord(EditorState& st);

char shift(char c);

void enterInsertMode(EditorState& st, InsertionPosition pos = InsertAtCursor);

void exitInsertMode(EditorState& st);

char eventToChar(SDL_Event& e);

void insertLineAtCurrLine(EditorState& st);

void insertLineAboveCurrLine(EditorState& st);

void handleNormalModeEvent(EditorState& st, SDL_Event& e);

void handleInsertModeKeyDown(EditorState& st, SDL_Event& e);

void handleInsertModeTextInput(EditorState& st, SDL_Event& e);

void handleVisualModeEvent(EditorState& st, SDL_Event& e);
