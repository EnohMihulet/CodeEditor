#pragma once
#include <cstdint>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float f32;
typedef double f64;

constexpr s16 OK = 0;
constexpr s16 ERR_UNKNOWN = -1;
constexpr s16 ERR_FILE_NOT_FOUND = -2;
constexpr s16 ERR_UNSUPPORTED = -3;
constexpr s16 ERR_EOF = -4;
