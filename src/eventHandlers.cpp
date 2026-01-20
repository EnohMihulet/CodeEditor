#include "eventHandlers.h"
#include "diagnostics.h"
#include <print>

static void recordKeyEvent(SDL_Event& e);
static void markDirty(EditorState& st);

SDL_Texture* resizeWindow(EditorState& st, u32 w, u32 h, SDL_Renderer* ren) {
	st.screenBuf.width = w;
	st.screenBuf.height = h;
	st.screenBuf.pitch = w * 4;

	SDL_free(st.screenBuf.pixels);
	st.screenBuf.pixels = (u32*)SDL_malloc((size_t)w * (size_t)h * 4);

	return SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
}

void moveDownOneLine(EditorState& st) {
	DIAG_ASSERT(st.CurrLine < st.Text->size, "moveDownOneLine current line out of range");
	if (st.CurrLine == st.Text->size - 1) return;

	st.CurrLine += 1;
	st.CurrLineBuffer = st.CurrLineBuffer ? st.CurrLineBuffer->next : nullptr;

	auto& lb = st.CurrLineBuffer;
	if (lb->size == 0) st.CursorPos = 0;
	else if (st.CursorPos > lb->size-1) st.CursorPos = lb->size-1;

	if (st.DisplayedLineCount < st.MaxDisplayedLineCount || st.BottomLine >= st.Text->size - 1) return;

	if ((s32)st.CurrLine + 14 >= (s32)st.BottomLine) {
		st.TopLine += 1;
		st.BottomLine += 1;
	}
}

void moveUpOneLine(EditorState& st) {
	if (st.CurrLine == 0) return;

	st.CurrLine -= 1;
	st.CurrLineBuffer = st.CurrLineBuffer ? st.CurrLineBuffer->prev : nullptr;

	auto& lb = st.CurrLineBuffer;
	if (lb->size == 0) st.CursorPos = 0;
	else if (st.CursorPos > lb->size-1) st.CursorPos = lb->size-1;

	if (st.DisplayedLineCount < st.MaxDisplayedLineCount || st.TopLine == 0) return;

	if ((s32)st.CurrLine - 14 <= (s32)st.TopLine) {
		st.TopLine -= 1;
		st.BottomLine -= 1;
	}
}

void jumpToLine(EditorState& st, u32 line) {
	if (line >= st.Text->size) line = st.Text->size - 1;

	st.CurrLine = line;

	if (line + st.DisplayedLineCount > st.Text->size) {
		st.BottomLine = line;
		st.TopLine = (line + 1 > st.DisplayedLineCount) ? line - (st.DisplayedLineCount - 1) : 0;
	} else {
	st.TopLine = line;
	st.BottomLine = line + st.DisplayedLineCount - 1;
	}

	st.CurrLineBuffer = st.Text->getLineBuffer(st.CurrLine);

	auto& lb = st.CurrLineBuffer;
	if (lb->size == 0) st.CursorPos = 0;
	else if (st.CursorPos > lb->size - 1) st.CursorPos = lb->size - 1;
}

void jumpToStartOfLine(EditorState& st) {
	st.CursorPos = 0;
}

void jumpToFirstNonWhitespace(EditorState& st) {
	auto& lb = st.CurrLineBuffer;
	if (!lb || lb->size == 0) {
		st.CursorPos = 0;
		return;
	}

	u32 pos = 0;
	while (pos < lb->size) {
		char c = lb->text[pos];
		if (c != ' ' && c != '\t') break;
		pos += 1;
	}

	st.CursorPos = (pos < lb->size) ? pos : 0;
}

void jumpToEndOfLine(EditorState& st) {
	auto& lb = st.CurrLineBuffer;
	st.CursorPos = lb->size == 0 ? 0 : lb->size - 1;
}

void jumpToTopOfWindow(EditorState& st) {
	jumpToLine(st, st.TopLine);
}

void jumpToMiddleOfWindow(EditorState& st) {
	u32 offset = st.DisplayedLineCount == 0 ? 0 : st.DisplayedLineCount / 2;
	jumpToLine(st, st.TopLine + offset);
}

void jumpToBottomOfWindow(EditorState& st) {
	jumpToLine(st, st.BottomLine);
}

void moveRight(EditorState& st) {
	auto& lb = st.CurrLineBuffer;
	st.CursorPos += (st.CursorPos == lb->size - 1) ? 0 : 1;
}

void moveLeft(EditorState& st) {
	st.CursorPos -= (st.CursorPos == 0) ? 0 : 1;
}

bool isBlank(u8 c) {
	return std::isspace(c) != 0;
}

bool isKeyword(u8 c) {
	return (std::isalnum(c) != 0) || c == '_';
}

s32 advance(EditorState& st) {
	auto& lb = st.CurrLineBuffer;
	if (st.CurrLine == st.Text->size - 1 && (lb->size == 0 || st.CursorPos == lb->size - 1)) return ERR_EOF;
	if (st.CursorPos == lb->size - 1) {
		moveDownOneLine(st);
		st.CursorPos = 0;
	}
	else st.CursorPos += 1;
	return OK;
}

s32 retreat(EditorState& st) {
	auto& lb = st.CurrLineBuffer;
	if (st.CurrLine == 0 && st.CursorPos == 0) return ERR_EOF;
	if (st.CursorPos == 0) {
		moveUpOneLine(st);
		st.CursorPos = lb->size == 0 ? 0 : lb->size - 1;
	}
	else st.CursorPos -= 1;
	return OK;

}

void jumpToNextWord(EditorState& st) {
	auto& lb = st.CurrLineBuffer;
	if (st.CurrLine == st.Text->size - 1 && (lb->size == 0 || st.CursorPos == lb->size - 1)) return;

	// If you are at the end of the line, move down lines until:
	// 1) you are at an empty line
	// 2) you are at a char that is not a tab
	if (st.CursorPos == lb->size - 1 || lb->size == 0) {
		moveDownOneLine(st);
		st.CursorPos = 0;
		if (lb->size == 0) return;

		while (lb->text[st.CursorPos] == '\t') {
			if (advance(st) == ERR_EOF) return;
		}
		return;
	}
	
	// If the current char is alpha-numeric, increase the cursor position until a symbol is reached
	// else (current char is symbol), increment cursor pos
	char currChar = lb->text[st.CursorPos];
	if (isKeyword(currChar)) {
		while (isKeyword(currChar)) {
			if (advance(st) == ERR_EOF) return;
			currChar = lb->text[st.CursorPos];
		}
	}
	else st.CursorPos += 1;

	// Do not end on an empty space or tab if you started on a alpha-numeric char or symbol
	currChar = lb->text[st.CursorPos];
	while (currChar == ' ' ||  currChar == '\t') {
		if (advance(st) == ERR_EOF) return;
		currChar = lb->text[st.CursorPos];
	}
	return;
}

void jumpToPrevWord(EditorState& st) {
	if (st.CurrLine == 0 && st.CursorPos == 0) return;
	auto& lb = st.CurrLineBuffer;

	// If you are at the start of the line, move up lines until:
	// 1) you are at an empty line
	// 2) you are at a char that is not a tab
	if (st.CursorPos == 0 || lb->size == 0) {
		moveUpOneLine(st);
		st.CursorPos = lb->size == 0 ? 0 : lb->size - 1;
		if (lb->size == 0) return;


		while (lb->text[st.CursorPos] == '\t') {
			if (retreat(st) == ERR_EOF) return;
		}
		return;
	}
	
	// If the current char is alpha-numeric, AND
	// 1) you are at first char in a string of keywords:
	// decrease until you reach a symbol or the start of a new string of keywords
	// 2) you are in not at the first char in a string of keywords:
	// decrease until you reach the start of the string of keywords
	char currChar = lb->text[st.CursorPos];
	if (isKeyword(currChar)) {
		if (retreat(st) == ERR_EOF) return;
		currChar = lb->text[st.CursorPos];

		while (currChar == ' ' ||  currChar == '\t') {
			if (retreat(st) == ERR_EOF) return;
			currChar = lb->text[st.CursorPos];
		}

		if (!isKeyword(currChar)) return;
		while (isKeyword(currChar)) {
			if (st.CursorPos == 0) return;
			if (retreat(st) == ERR_EOF) return;
			currChar = lb->text[st.CursorPos];
		}
		advance(st);
	}
	// else decrease until you are at a symbol or start of a string of keywords
	else {
		st.CursorPos -= 1;
		currChar = lb->text[st.CursorPos];
		while (currChar == ' ' ||  currChar == '\t') {
			if (retreat(st) == ERR_EOF) return;
			currChar = lb->text[st.CursorPos];
		}

		if (isKeyword(currChar)) {
			while (isKeyword(currChar)) {
				if (st.CursorPos == 0) return;
				if (retreat(st) == ERR_EOF) return;
				currChar = lb->text[st.CursorPos];
			}
			advance(st);
		}
	}
	return;
}

void jumpToEndOfNextWord(EditorState& st) {
	auto& lb = st.CurrLineBuffer;
	if (st.CurrLine == st.Text->size - 1 && (lb->size == 0 || st.CursorPos == lb->size - 1)) return;

	// If you are at the end of the line, move down lines until:
	// 1) you are at an empty line
	// 2) you are at a char that is not a tab
	if (st.CursorPos == lb->size - 1 || lb->size == 0) {
		moveDownOneLine(st);
		st.CursorPos = 0;
		if (lb->size == 0) return;

		while (lb->text[st.CursorPos] == '\t') {
			if (advance(st) == ERR_EOF) return;
		}
		return;
	}
	
	// If the current char is alpha-numeric, increase the cursor position until a symbol is reached
	char currChar = lb->text[st.CursorPos];
	if (isKeyword(currChar)) {
		if (advance(st) == ERR_EOF) return;
		currChar = lb->text[st.CursorPos];

		while (currChar == ' ' ||  currChar == '\t') {
			if (advance(st) == ERR_EOF) return;
			currChar = lb->text[st.CursorPos];
		}

		if (!isKeyword(currChar)) return;
		while (isKeyword(currChar)) {
			if (st.CursorPos == 0) return;
			if (advance(st) == ERR_EOF) return;
			currChar = lb->text[st.CursorPos];
		}
		retreat(st);
	}
	// else (current char is symbol), increment cursor pos
	else {
		st.CursorPos += 1;
		currChar = lb->text[st.CursorPos];
		while (currChar == ' ' ||  currChar == '\t') {
			if (advance(st) == ERR_EOF) return;
			currChar = lb->text[st.CursorPos];
		}

		if (isKeyword(currChar)) {
			while (isKeyword(currChar)) {
				if (st.CursorPos == 0) return;
				if (advance(st) == ERR_EOF) return;
				currChar = lb->text[st.CursorPos];
			}
			retreat(st);
		}
	}
	return;
}


char shift(char c) {
	if (c >= 'a' && c <= 'z') return c - ('a' - 'A');

	switch (c) {
		case '1': return '!';
		case '2': return '@';
		case '3': return '#';
		case '4': return '$';
		case '5': return '%';
		case '6': return '^';
		case '7': return '&';
		case '8': return '*';
		case '9': return '(';
		case '0': return ')';

		case '-': return '_';
		case '=': return '+';
		case '[': return '{';
		case ']': return '}';
		case '\\': return '|';
		case ';': return ':';
		case '\'': return '"';
		case ',': return '<';
		case '.': return '>';
		case '/': return '?';
		case '`': return '~';

		case ' ': return ' ';
		case '\t': return '\t';
		case '\n': return '\n';

		default: return c;
	}
}

void enterInsertMode(EditorState& st, InsertionPosition pos) {
	st.CurrMode = InsertMode;
	SDL_StartTextInput();
	SDL_FlushEvent(SDL_TEXTINPUT);

	DIAG_ASSERT(st.CurrLineBuffer, "enterInsertMode missing line buffer");
	auto& lb = st.CurrLineBuffer;
	if (pos == InsertAfterCursor) {
		if (st.CursorPos < lb->size) st.CursorPos += 1;
		else st.CursorPos = lb->size;
	}
}

void exitInsertMode(EditorState& st) {
	st.CurrMode = NormalMode;
	auto& lb = st.CurrLineBuffer;
	if (st.CursorPos == lb->size && st.CursorPos != 0) st.CursorPos-=1;
	SDL_StopTextInput();
}

char eventToChar(SDL_Event& e) {
	char c = 0;
	switch (e.key.keysym.sym) {
		case SDLK_RETURN:
		case SDLK_RETURN2: c = '\n'; break;
		case SDLK_TAB: c = '\t'; break;
		case SDLK_BACKSPACE: c = '\b'; break;
		case SDLK_SPACE: c = ' '; break;

		case SDLK_EXCLAIM: c = '!'; break;
		case SDLK_QUOTEDBL: c = '"'; break;
		case SDLK_HASH: c = '#'; break;
		case SDLK_PERCENT: c = '%'; break;
		case SDLK_DOLLAR: c = '$'; break;
		case SDLK_AMPERSAND: c = '&'; break;
		case SDLK_QUOTE: c = '\''; break;
		case SDLK_LEFTPAREN: c = '('; break;
		case SDLK_RIGHTPAREN: c = ')'; break;
		case SDLK_ASTERISK: c = '*'; break;
		case SDLK_PLUS: c = '+'; break;
		case SDLK_COMMA: c = ','; break;
		case SDLK_MINUS: c = '-'; break;
		case SDLK_PERIOD: c = '.'; break;
		case SDLK_SLASH: c = '/'; break;

		case SDLK_0: c = '0'; break;
		case SDLK_1: c = '1'; break;
		case SDLK_2: c = '2'; break;
		case SDLK_3: c = '3'; break;
		case SDLK_4: c = '4'; break;
		case SDLK_5: c = '5'; break;
		case SDLK_6: c = '6'; break;
		case SDLK_7: c = '7'; break;
		case SDLK_8: c = '8'; break;
		case SDLK_9: c = '9'; break;

		case SDLK_COLON: c = ':'; break;
		case SDLK_SEMICOLON: c = ';'; break;
		case SDLK_LESS: c = '<'; break;
		case SDLK_EQUALS: c = '='; break;
		case SDLK_GREATER: c = '>'; break;
		case SDLK_QUESTION: c = '?'; break;
		case SDLK_AT: c = '@'; break;
		case SDLK_LEFTBRACKET: c = '['; break;
		case SDLK_BACKSLASH: c = '\\'; break;
		case SDLK_RIGHTBRACKET: c = ']'; break;
		case SDLK_CARET: c = '^'; break;
		case SDLK_UNDERSCORE: c = '_'; break;
		case SDLK_BACKQUOTE: c = '`'; break;

		case SDLK_a: c = 'a'; break;
		case SDLK_b: c = 'b'; break;
		case SDLK_c: c = 'c'; break;
		case SDLK_d: c = 'd'; break;
		case SDLK_e: c = 'e'; break;
		case SDLK_f: c = 'f'; break;
		case SDLK_g: c = 'g'; break;
		case SDLK_h: c = 'h'; break;
		case SDLK_i: c = 'i'; break;
		case SDLK_j: c = 'j'; break;
		case SDLK_k: c = 'k'; break;
		case SDLK_l: c = 'l'; break;
		case SDLK_m: c = 'm'; break;
		case SDLK_n: c = 'n'; break;
		case SDLK_o: c = 'o'; break;
		case SDLK_p: c = 'p'; break;
		case SDLK_q: c = 'q'; break;
		case SDLK_r: c = 'r'; break;
		case SDLK_s: c = 's'; break;
		case SDLK_t: c = 't'; break;
		case SDLK_u: c = 'u'; break;
		case SDLK_v: c = 'v'; break;
		case SDLK_w: c = 'w'; break;
		case SDLK_x: c = 'x'; break;
		case SDLK_y: c = 'y'; break;
		case SDLK_z: c = 'z'; break;
	}

	bool shiftHeld = (e.key.keysym.mod & KMOD_SHIFT) != 0;
	bool capsLock = (e.key.keysym.mod & KMOD_CAPS)  != 0;
	bool shouldShift = shiftHeld ^ capsLock;

	return shouldShift ? shift(c) : c;
}

void recordKeyEvent(SDL_Event& e) {
	if (e.type == SDL_TEXTINPUT) {
		for (u16 i = 0; e.text.text[i] != '\0'; i++) {
			diagRecordKeyChar(e.text.text[i]);
		}
		return;
	}

	if (e.type != SDL_KEYDOWN) return;

	char c = eventToChar(e);
	if (c != 0) diagRecordKeyChar(c);
	else diagRecordKey(SDL_GetKeyName(e.key.keysym.sym));
}

static void markDirty(EditorState& st) {
	st.isDirty = true;
}

void insertLineAtCurrLine(EditorState& st) {
	st.Text->insertAtLine(st.CurrLineBuffer->next);
	markDirty(st);
	if (st.DisplayedLineCount != st.MaxDisplayedLineCount) {
		st.DisplayedLineCount += 1;
		st.BottomLine += 1;
	}
	moveDownOneLine(st);
	enterInsertMode(st);
}

void insertLineAboveCurrLine(EditorState& st) {
	auto& lb = st.CurrLineBuffer;
	st.Text->insertAtLine(lb);
	markDirty(st);
	if (st.DisplayedLineCount != st.MaxDisplayedLineCount) {
		st.DisplayedLineCount += 1;
		st.BottomLine += 1;
	}
	if (st.CurrLine != 1) {
		moveUpOneLine(st);
		st.CurrLine += 1;
	}
	else st.CurrLineBuffer = lb->prev;
	enterInsertMode(st);
}

void splitLineAtCursor(EditorState& st) {
	auto& lb = st.CurrLineBuffer;
	st.Text->insertAtLine(st.CurrLineBuffer->next);
	markDirty(st);
	if (st.DisplayedLineCount != st.MaxDisplayedLineCount) {
		st.DisplayedLineCount += 1;
		st.BottomLine += 1;
	}
	lb->splitAt(st.CursorPos, lb->next);
	moveDownOneLine(st);
	st.CursorPos = 0;
}

void handleNormalModeEvent(EditorState& st, SDL_Event& e) {
	recordKeyEvent(e);
	char c = eventToChar(e);
	static bool awaitingSecondG = false;

	if (c == 'g') {
		if (awaitingSecondG) {
			jumpToLine(st, 0);
			awaitingSecondG = false;
		} else {
			awaitingSecondG = true;
		}
		return;
	}

	awaitingSecondG = false;
	auto& lb = st.CurrLineBuffer;
	switch (c) {
		case 'k': moveUpOneLine(st); break;
		case 'j': moveDownOneLine(st); break;
		case '1': DIAG_ASSERT(false, "Test 1");
		case 'h': moveLeft(st); break;
		case 'l': moveRight(st); break;
		case '0': jumpToStartOfLine(st); break;
		case '^': jumpToFirstNonWhitespace(st); break;
		case 'G': jumpToLine(st, st.Text->size-1); break;
		case 'H': jumpToTopOfWindow(st); break;
		case 'M': jumpToMiddleOfWindow(st); break;
		case 'L': jumpToBottomOfWindow(st); break;
		case 'a': enterInsertMode(st, InsertAfterCursor); break;
		case 'i': enterInsertMode(st, InsertAtCursor); break;
		case 'w': jumpToNextWord(st); break;
		case 'b': jumpToPrevWord(st); break;
		case 'e': jumpToEndOfNextWord(st); break;
		case '$': jumpToEndOfLine(st); break;
		case 'v': st.CurrMode = VisualMode; break;
		case 'o': { insertLineAtCurrLine(st); } break;
		case 'O': { insertLineAboveCurrLine(st); } break;
		case 'x': { 
			lb->removeAt(st.CursorPos);
			markDirty(st);
			if (st.CursorPos != 0 && st.CursorPos == lb->size) st.CursorPos -= 1;
		} break;
		default: break;
	}
}

void handleInsertModeKeyDown(EditorState& st, SDL_Event& e) {
	recordKeyEvent(e);
	if (!st.CurrLineBuffer) return;
	auto& lb = st.CurrLineBuffer;

	switch (e.key.keysym.sym) {
		case SDLK_ESCAPE: exitInsertMode(st); break;
		case SDLK_BACKSPACE: {
			if (st.CursorPos != 0) {
				lb->removeAt(st.CursorPos - 1);
				st.CursorPos -= 1;
				markDirty(st);
			}
		} break;
		case SDLK_RETURN:
		case SDLK_RETURN2: splitLineAtCursor(st); break;
		case SDLK_TAB: lb->appendAt('\t', st.CursorPos); st.CursorPos += 1; markDirty(st); break;

		default: break;
	}
}

void handleInsertModeTextInput(EditorState& st, SDL_Event& e) {
	recordKeyEvent(e);
	if (e.key.keysym.sym == SDLK_ESCAPE) {
		exitInsertMode(st);
		return;
	}

	if (!st.CurrLineBuffer) return;
	auto& lb = st.CurrLineBuffer;

	for (u16 i = 0; e.text.text[i] != '\0'; i++) {
		lb->appendAt(e.text.text[i], st.CursorPos);
		st.CursorPos++;
	}
	if (e.text.text[0] != '\0') markDirty(st);
}

void handleVisualModeEvent(EditorState& st, SDL_Event& e) {
	recordKeyEvent(e);
	switch (e.key.keysym.sym) {
		case SDLK_ESCAPE: st.CurrMode = NormalMode; break;
		default: break;
	}
}
