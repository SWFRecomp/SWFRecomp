#pragma once

#include <cstdio>
#include <cstdint>
#include <exception>

#define EXC(str) fprintf(stderr, str); throw std::exception();
#define EXC_ARG(str, arg) fprintf(stderr, str, arg); throw std::exception();

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;