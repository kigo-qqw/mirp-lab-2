#include <string/to_string.h>

#include <stdlib.h>
#include <string.h>

#include <common/types.h>
#include <string/to_string_impl.h>

void FreeConstString(const c8 *const *const p) { free((void *)*p); }

void FreeMutableString(c8 *const *p) { free(*p); }

const c8 *NullSafetyToString(const void *const Self,
                             const ToString ToStringMethod) {
  if (Self == NULLPTR) {
    return strdup("null");
  }
  return ToStringMethod(Self);
}

TO_STRING_DEFINITION(u64, {}, "%lu", (const i64)u64); // TODO: fixme
