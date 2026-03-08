#pragma once

#include <stdatomic.h>
#include <stdnoreturn.h>

#if __STDC_VERSION__ >= 202311L
#define NULLPTR nullptr
#define NORETURN [[noreturn]]
#define FALLTHROUGH [[fallthrough]];
#else
#define NULLPTR NULL
#define NORETURN noreturn
#define FALLTHROUGH __attribute__((fallthrough));
#endif

#define Atomic _Atomic

#define AUTO(FREE_METHOD) __attribute__((cleanup(FREE_METHOD)))
