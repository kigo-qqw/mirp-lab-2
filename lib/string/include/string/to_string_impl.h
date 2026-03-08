#pragma once

#define TO_STRING_DEFINITION(T, DEFER_CLEANUP, FMT, ...)                       \
  const char *TO_STRING(T)(const void *const Self) {                           \
    const T *const T = Self;                                                   \
    c8 *Buffer = NULL;                                                         \
    usize BufferSize = 0;                                                      \
                                                                               \
    DEFER_CLEANUP;                                                             \
                                                                               \
    while (true) {                                                             \
      BufferSize =                                                             \
          (usize)snprintf(Buffer, BufferSize, FMT, ##__VA_ARGS__) + 1;         \
      if (Buffer)                                                              \
        return Buffer;                                                         \
      Buffer = malloc(BufferSize);                                             \
    }                                                                          \
  }
