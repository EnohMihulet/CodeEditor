#pragma once
#include <cstddef>

void diagRecordKey(const char* key);
void diagRecordKeyChar(char c);
[[noreturn]] void diagAbort(const char* expr, const char* file, int line, const char* msg);

#if defined(RELEASE)
#define DIAG_ASSERT(expr, msg) do { if (!(expr)) diagAbort(#expr, __FILE__, __LINE__, msg); } while (0)
#else
#include <cassert>
#define DIAG_ASSERT(expr, msg) assert(expr)
#endif
