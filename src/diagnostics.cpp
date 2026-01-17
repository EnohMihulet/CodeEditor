#include "diagnostics.h"

#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace {
	constexpr size_t kMaxKeys = 16;
	constexpr size_t kKeyLen = 16;

	std::array<std::array<char, kKeyLen>, kMaxKeys> g_keys{};
	size_t g_keyCount = 0;
	size_t g_keyIndex = 0;

	void storeKey(std::string_view key) {
		if (key.empty()) return;

		auto& slot = g_keys[g_keyIndex];
		size_t n = key.size();
		if (n >= kKeyLen) n = kKeyLen - 1;
		std::memcpy(slot.data(), key.data(), n);
		slot[n] = '\0';

		g_keyIndex = (g_keyIndex + 1) % kMaxKeys;
		if (g_keyCount < kMaxKeys) g_keyCount += 1;
	}

	void writeRecentKeys(FILE* out) {
		if (g_keyCount == 0) {
			std::fputs("none", out);
			return;
		}

		size_t start = (g_keyCount < kMaxKeys) ? 0 : g_keyIndex;
		for (size_t i = 0; i < g_keyCount; i++) {
			size_t idx = (start + i) % kMaxKeys;
			std::fputs(g_keys[idx].data(), out);
			if (i + 1 < g_keyCount) std::fputc(' ', out);
		}
	}
}

void diagRecordKey(const char* key) {
	if (!key) return;
	storeKey(key);
}

void diagRecordKeyChar(char c) {
	if (c == '\n') storeKey("\\n");
	else if (c == '\t') storeKey("\\t");
	else if (c == '\b') storeKey("\\b");
	else if (std::isprint(static_cast<unsigned char>(c))) {
		char buf[2]{c, '\0'};
		storeKey(buf);
	}
	else {
		char buf[8]{};
		std::snprintf(buf, sizeof(buf), "0x%02X", static_cast<unsigned char>(c));
		storeKey(buf);
	}
}

[[noreturn]] void diagAbort(const char* expr, const char* file, int line, const char* msg) {
	std::fprintf(stderr, "Fatal error: %s\n", msg ? msg : "unknown");
	std::fprintf(stderr, "Assertion: %s\n", expr ? expr : "(none)");
	std::fprintf(stderr, "Location: %s: %d\n", file ? file : "(unknown)", line);
	std::fputs("Recent keys: ", stderr);
	writeRecentKeys(stderr);
	std::fputc('\n', stderr);
	std::fflush(stderr);
	std::abort();
}
