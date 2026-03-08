#include <vector/vector.h>

#include <stdio.h>
#include <string.h>

#include <common/types.h>

#define LEFT_BRACE "["
#define RIGHT_BRACE "]"
#define ELEMENT_DELIMITER ", "

char *Vector_ToString_Generic(const void *Data, const usize Size,
                              const usize ElementSize,
                              const char *(*ToString)(const void *)) {
  if (!Data)
    return strdup("null");
  if (Size == 0)
    return strdup(LEFT_BRACE RIGHT_BRACE);

  const char **const ElementsAsStrings = malloc(sizeof(char *) * Size);

  // null-terminator + braces
  usize TotalBufferSize = 1 + sizeof(LEFT_BRACE) - 1 + sizeof(RIGHT_BRACE) - 1;

  for (usize i = 0; i < Size; ++i) {
    if (i > 0)
      TotalBufferSize += sizeof(ELEMENT_DELIMITER) - 1;

    const void *Element = *(const void **)((const c8 *)Data + i * ElementSize);
    ElementsAsStrings[i] = ToString(Element);
    TotalBufferSize += strlen(ElementsAsStrings[i]);
  }

  c8 *const Buffer = malloc(TotalBufferSize);
  usize Length = 0;
  Length +=
      (usize)snprintf(Buffer + Length, TotalBufferSize - Length, LEFT_BRACE);

  for (usize i = 0; i < Size; ++i) {
    if (i > 0)
      Length += (usize)snprintf(Buffer + Length, TotalBufferSize - Length,
                                ELEMENT_DELIMITER);

    Length += (usize)snprintf(Buffer + Length, TotalBufferSize - Length, "%s",
                              ElementsAsStrings[i]);
    free((void *)ElementsAsStrings[i]);
  }
  snprintf(Buffer + Length, TotalBufferSize - Length, RIGHT_BRACE);

  free(ElementsAsStrings);
  return Buffer;
}
