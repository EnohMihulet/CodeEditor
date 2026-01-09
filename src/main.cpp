#include <cassert>
#include <print>
#include <string>
#include <cstring>

#include <SDL.h>

#include "commonTypes.h"
#include "config.h"
#include "render.h"

SDL_Texture* resizeWindow(EditorState& st, u32 w, u32 h, SDL_Renderer* ren) {
	st.screenBuf.width = w;
	st.screenBuf.height = h;
	st.screenBuf.pitch = w * 4;

	SDL_free(st.screenBuf.pixels);
	st.screenBuf.pixels = (u32*)SDL_malloc((size_t)w * (size_t)h * 4);

	return SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
}

f64 getDeltaTimeSeconds() {
	static u64 last = 0;
	u64 now = SDL_GetPerformanceCounter();
	f64 dt = last ? (double)(now - last) / (double)SDL_GetPerformanceFrequency() : 0.0;
	last = now;
	return dt;
}

static void moveUpOneLine(EditorState& st) {
	if (st.CurrLine == 0) return;

	st.CurrLine -= 1;
	st.CurrLineBuffer = st.CurrLineBuffer ? st.CurrLineBuffer->prev : nullptr;

	if (st.CurrLineBuffer->size == 0) st.CursorPos = 0;
	else if (st.CursorPos > st.CurrLineBuffer->size-1) st.CursorPos = st.CurrLineBuffer->size-1;

	if (st.DisplayedLineCount < st.MaxDisplayedLineCount || st.TopLine == 0) return;

	if ((s32)st.CurrLine - 14 <= (s32)st.TopLine) {
		st.TopLine -= 1;
		st.BottomLine -= 1;
	}
}

static void moveDownOneLine(EditorState& st) {
	assert(st.CurrLine < st.Text.size);
	if (st.CurrLine == st.Text.size - 1) return;

	st.CurrLine += 1;
	st.CurrLineBuffer = st.CurrLineBuffer ? st.CurrLineBuffer->next : nullptr;

	if (st.CurrLineBuffer->size == 0) st.CursorPos = 0;
	else if (st.CursorPos > st.CurrLineBuffer->size-1) st.CursorPos = st.CurrLineBuffer->size-1;

	if (st.DisplayedLineCount < st.MaxDisplayedLineCount || st.BottomLine >= st.Text.size - 1) return;

	if ((s32)st.CurrLine + 14 >= (s32)st.BottomLine) {
		st.TopLine += 1;
		st.BottomLine += 1;
	}
}

static void jumpToLine(EditorState& st, u32 line) {
	if (line >= st.Text.size) line = st.Text.size - 1;

	st.CurrLine = line;

	if (line + st.DisplayedLineCount > st.Text.size) {
		st.BottomLine = line;
		st.TopLine = (line + 1 > st.DisplayedLineCount) ? line - (st.DisplayedLineCount - 1) : 0;
	} else {
		st.TopLine = line;
		st.BottomLine = line + st.DisplayedLineCount - 1;
	}

	st.CurrLineBuffer = st.Text.getLineBuffer(st.CurrLine);
}

static char shift(char c) {
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

static void enterInsertMode(EditorState& st) {
	st.CurrMode = InsertMode;
	SDL_StartTextInput();
	SDL_FlushEvent(SDL_TEXTINPUT);
}

static void exitInsertMode(EditorState& st) {
	st.CurrMode = NormalMode;
	if (st.CursorPos == st.CurrLineBuffer->size && st.CursorPos != 0) st.CursorPos-=1;
	SDL_StopTextInput();
}

static char eventToChar(SDL_Event& e) {
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

static void insertLineAtCurrLine(EditorState& st) {
	st.Text.insertAtLine(st.CurrLineBuffer->next);
	if (st.DisplayedLineCount != st.MaxDisplayedLineCount) {
		st.DisplayedLineCount += 1;
		st.BottomLine += 1;
	}
	moveDownOneLine(st);
	enterInsertMode(st);
}

static void insertLineAboveCurrLine(EditorState& st) {
	st.Text.insertAtLine(st.CurrLineBuffer);
	if (st.DisplayedLineCount != st.MaxDisplayedLineCount) {
		st.DisplayedLineCount += 1;
		st.BottomLine += 1;
	}
	if (st.CurrLine != 1) {
		moveUpOneLine(st);
		st.CurrLine += 1;
	}
	else st.CurrLineBuffer = st.CurrLineBuffer->prev;
	enterInsertMode(st);
}

static void handleNormalModeEvent(EditorState& st, SDL_Event& e) {
	char c = eventToChar(e);
	switch (c) {
		case 'k': moveUpOneLine(st); break;
		case 'j': moveDownOneLine(st); break;
		case 'h': st.CursorPos -= (st.CursorPos == 0) ? 0 : 1; break;
		case 'l': st.CursorPos += (st.CursorPos == st.CurrLineBuffer->size - 1) ? 0 : 1; break;
		case 'q': jumpToLine(st, st.Text.size-1); break;
		case 'a': enterInsertMode(st); st.CursorPos++; break;
		case 'i': enterInsertMode(st); break;
		case 'v': st.CurrMode = VisualMode; break;
		case 'o': { insertLineAtCurrLine(st); } break;
		case 'O': { insertLineAboveCurrLine(st); } break;
		case 'x': { st.CurrLineBuffer->removeAt(0); } break;
		default: break;
	}
}

static void handleInsertModeKeyDown(EditorState& st, SDL_Event& e) {
	if (!st.CurrLineBuffer) return;

	switch (e.key.keysym.sym) {
		case SDLK_ESCAPE: exitInsertMode(st); break;
		case SDLK_BACKSPACE: st.CurrLineBuffer->remove(); st.CursorPos -= (st.CursorPos == 0) ? 0 : 1; break;
		case SDLK_RETURN:
		case SDLK_RETURN2: insertLineAtCurrLine(st); break;
		case SDLK_TAB: st.CurrLineBuffer->append('\t'); break;

		default: break;
	}
}

static void handleInsertModeTextInput(EditorState& st, SDL_Event& e) {
	if (e.key.keysym.sym == SDLK_ESCAPE) {
		exitInsertMode(st);
		return;
	}

	if (!st.CurrLineBuffer) return;

	for (u16 i = 0; i < e.text.text[i] != '\0'; i++) {
		st.CurrLineBuffer->appendAt(e.text.text[i], st.CursorPos);
		st.CursorPos++;
	}
}

static void handleVisualModeEvent(EditorState& st, SDL_Event& e) {
	switch (e.key.keysym.sym) {
		case SDLK_ESCAPE: st.CurrMode = NormalMode; break;
		default: break;
	}
}

s32 main() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		std::println("SDL_Init failed.");
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"CodeEditor",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_RESIZABLE
	);
	if (!window) {
		std::println("SDL_CreateWindow failed.");
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		std::println("SDL_CreateRenderer failed.");
		return -1;
	}

	SDL_Texture* texture = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
		WINDOW_WIDTH, WINDOW_HEIGHT
	);
	if (!texture) {
		std::println("SDL_CreateTexture failed.");
		return -1;
	}

	EditorState st{};
	st.screenBuf = { WINDOW_WIDTH, WINDOW_HEIGHT, 4 * WINDOW_WIDTH,
					 (u32*)SDL_malloc((size_t)WINDOW_WIDTH * (size_t)WINDOW_HEIGHT * 4) };

	st.MaxDisplayedLineCount = (st.screenBuf.height - TPAD - BPAD) / LINE_HEIGHT;
	st.DisplayedLineCount = std::min(st.Text.size, st.MaxDisplayedLineCount);
	st.BottomLine = st.DisplayedLineCount - 1;
	st.CurrLineBuffer = st.Text.getLineBuffer(st.CurrLine);

	if (loadFnt(st.Font, FNT_PATH) != 0) {
		std::println("loadFnt failed.");
		return -1;
	}
	if (loadTga(st.TgaFontImg, TGA_PATH) != 0) {
		std::println("loadTga failed.");
		return -1;
	}

	bool running = true;
	while (running) {
		double dt = getDeltaTimeSeconds();
		double fps = (dt > 0.0) ? (1.0 / dt) : 0.0;

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) running = false;

			if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				SDL_DestroyTexture(texture);
				texture = resizeWindow(st, (u32)e.window.data1, (u32)e.window.data2, renderer);

				st.MaxDisplayedLineCount = (st.screenBuf.height - TPAD - BPAD) / LINE_HEIGHT;
				st.DisplayedLineCount = std::min(st.Text.size, st.MaxDisplayedLineCount);
				st.BottomLine = st.TopLine + st.DisplayedLineCount - 1;
				if (st.BottomLine >= st.Text.size) st.BottomLine = st.Text.size - 1;
			}

			if (st.CurrMode == InsertMode) {
        			if (e.type == SDL_TEXTINPUT) {
        				handleInsertModeTextInput(st, e);
        			} else if (e.type == SDL_KEYDOWN) {
        				handleInsertModeKeyDown(st, e);
        			}
			}
			else {
				if (e.type == SDL_KEYDOWN) {
					if (st.CurrMode == NormalMode) handleNormalModeEvent(st, e);
					else if (st.CurrMode == VisualMode) handleVisualModeEvent(st, e);
				}
			}
		}

		renderLineSeparators(st);
		renderLineNumbers(st);
		renderTextBuffer(st);
		renderSelectedPosition(st, WHITE);
		renderCurrentMode(st);
		renderFPS(st, (u32)fps);

		SDL_UpdateTexture(texture, nullptr, st.screenBuf.pixels, (int)st.screenBuf.pitch);
		std::memset(st.screenBuf.pixels, 0, (size_t)st.screenBuf.pitch * (size_t)st.screenBuf.height);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
	}

	SDL_free(st.screenBuf.pixels);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
