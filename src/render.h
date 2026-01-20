#pragma once
#include "config.h"
#include "color.h"

void renderCharacter(EditorState& st, char character, u32 xPos, u32 yPos, u32 color);

void renderNumber(EditorState& st, std::string_view number, u32 xPos, u32 yPos, u32 color);

void renderString(EditorState& st, std::string_view str, u32 xPos, u32 yPos, u32 color);

void renderLineNumbers(EditorState& st);

void renderLineSeparators(EditorState& st);

void renderTextBuffer(EditorState& st);

void renderSelectedLine(EditorState& st);

void renderSelectedPosition(EditorState& st, u32 color);

void renderBottom(EditorState& st, u32 fps);
