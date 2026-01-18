#include <cassert>
#include <print>
#include <string>
#include <cstring>

#include <SDL.h>

#include "eventHandlers.h"
#include "render.h"
#include "fileLoader.h"

f64 getDeltaTimeSeconds() {
	static u64 last = 0;
	u64 now = SDL_GetPerformanceCounter();
	f64 dt = last ? (double)(now - last) / (double)SDL_GetPerformanceFrequency() : 0.0;
	last = now;
	return dt;
}

s32 main() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		std::println("SDL_Init failed.");
		return ERR_UNKNOWN;
	}

	SDL_Window* window = SDL_CreateWindow(
		"CodeEditor",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_RESIZABLE
	);
	if (!window) {
		std::println("SDL_CreateWindow failed.");
		return ERR_UNKNOWN;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		std::println("SDL_CreateRenderer failed.");
		return ERR_UNKNOWN;
	}

	SDL_Texture* texture = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
		WINDOW_WIDTH, WINDOW_HEIGHT
	);
	if (!texture) {
		std::println("SDL_CreateTexture failed.");
		return ERR_UNKNOWN;
	}

	EditorState st{};
	st.screenBuf = { WINDOW_WIDTH, WINDOW_HEIGHT, 4 * WINDOW_WIDTH,
					 (u32*)SDL_malloc((size_t)WINDOW_WIDTH * (size_t)WINDOW_HEIGHT * 4) };

	if (loadFile(st, FILE_PATH) != 0) {
		std::println("loadFile failed.");
		return ERR_FILE_NOT_FOUND;
	};
	st.MaxDisplayedLineCount = (st.screenBuf.height - TPAD - BPAD) / LINE_HEIGHT;
	st.DisplayedLineCount = std::min(st.Text->size, st.MaxDisplayedLineCount);
	st.BottomLine = st.DisplayedLineCount - 1;
	st.CurrLineBuffer = st.Text->getLineBuffer(st.CurrLine);

	s16 fontResult = loadFnt(st.Font, FNT_PATH);
	if (fontResult != OK) {
		std::println("loadFnt failed.");
		return fontResult;
	}
	s16 tgaResult = loadTga(st.TgaFontImg, TGA_PATH);
	if (tgaResult != OK) {
		std::println("loadTga failed.");
		return tgaResult;
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
				st.DisplayedLineCount = std::min(st.Text->size, st.MaxDisplayedLineCount);
				st.BottomLine = st.TopLine + st.DisplayedLineCount - 1;
				if (st.BottomLine >= st.Text->size) st.BottomLine = st.Text->size - 1;
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

		renderSelectedLine(st);
		renderLineSeparators(st);
		renderLineNumbers(st);
		renderSelectedPosition(st, GRAY_70);
		renderTextBuffer(st);
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
	return OK;
}
