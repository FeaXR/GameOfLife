#pragma once

// Portable debugger break
#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define DEBUG_BREAK() __builtin_trap()
#else
#include <stdlib.h>
#define DEBUG_BREAK() abort()
#endif

// If X is false, break (only in debug builds; release builds just evaluate X)
#ifdef NDEBUG
#define ASSERT(x) ((void)(x))
#else
#define ASSERT(x) do { if (!(x)) DEBUG_BREAK(); } while (0)
#endif

//Clear errors before function X, and check if after X any errors emerged
#define GLCall(x) GLClearError(); \
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))
