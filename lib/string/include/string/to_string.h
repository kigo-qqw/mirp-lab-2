#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <common/common.h>
#include <common/types.h>

void FreeConstString(const c8 *const *p);
void FreeMutableString(c8 *const *p);

#define String const c8 *AUTO(FreeConstString)
#define MutableString c8 *AUTO(FreeMutableString)

#define TO_STRING(T) T##ToString

#define TO_STRING_DECL(T) const c8 *TO_STRING(T)(const void *Self)

typedef const c8 *(*ToString)(const void *);

const c8 *NullSafetyToString(const void *Self, ToString ToStringMethod);

TO_STRING_DECL(u64);

#define btoa(x) ((x) ? "true" : "false")
