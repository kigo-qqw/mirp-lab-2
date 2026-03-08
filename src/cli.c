#include "cli.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string/to_string_impl.h>

static const c8 *const ShortOptions = "huvt::r::";

static struct option LongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"usage", no_argument, NULL, 'u'},
    {"version", no_argument, NULL, 'v'},
    {"trace", optional_argument, NULLPTR, 't'},
    {"run", optional_argument, NULLPTR, 'r'},
    {NULLPTR, 0, NULLPTR, 0}};

typedef struct ParseUnsignedIntegerResult {
  u64 Value;
  bool Success;
} ParseUnsignedIntegerResult;

static ParseUnsignedIntegerResult ParseUnsignedInteger(const c8 *Str) {
  c8 *EndPtr = NULL;

  ParseUnsignedIntegerResult Result = {strtoull(Str, &EndPtr, 10), true};

  if (errno == ERANGE) {
    errno = 0;
    fprintf(stderr, "Failed to parse %.*s", (int)(EndPtr - Str), Str);
    Result.Success = false;
    return Result;
  }

  return Result;
}

#define TRACE_LEVEL_NONE_STRING "NONE"
#define TRACE_LEVEL_RESULT_STRING "RESULT"
#define TRACE_LEVEL_ALL_STRING "ALL"

TO_STRING_DECL(TraceLevel) {
  const TraceLevel TL = *(const TraceLevel *)Self;
  const c8 *Result = "UNKNOWN";
  switch (TL) {
  case TraceLevel_NONE: {
    Result = "NONE";
  } break;
  case TraceLevel_RESULT: {
    Result = "RESULT";
  } break;
  case TraceLevel_ALL: {
    Result = "ALL";
  } break;
  }
  return strdup(Result);
}

typedef struct ParseTraceLevelResult {
  TraceLevel Value;
  bool Success;
} ParseTraceLevelResult;

static ParseTraceLevelResult ParseTraceLevel(const c8 *Str) {
  ParseTraceLevelResult Result = {.Value = TraceLevel_NONE, .Success = true};
  if (!Str) {
    Result.Value = TraceLevel_RESULT;
    return Result;
  }
  if (strcasecmp(Str, TRACE_LEVEL_NONE_STRING) == 0) {
    return Result;
  }
  if (strcasecmp(Str, TRACE_LEVEL_RESULT_STRING) == 0) {
    Result.Value = TraceLevel_RESULT;
    return Result;
  }
  if (strcasecmp(Str, TRACE_LEVEL_ALL_STRING) == 0) {
    Result.Value = TraceLevel_ALL;
    return Result;
  }
  Result.Success = false;
  return Result;
}

#define BENCHMARK_TYPE_SEQUENTIAL_STRING "SEQUENTIAL"
#define BENCHMARK_TYPE_SEQUENTIAL_CACHED_STRING "SEQUENTIAL_CACHED"
#define BENCHMARK_TYPE_PARALLEL_STRING "PARALLEL"
#define BENCHMARK_TYPE_PARALLEL_CACHED_STRING "PARALLEL_CACHED"

typedef struct BenchmarkMapEntry {
  const c8 *AsString;
  BenchmarkType Type;
} BenchmarkMapEntry;

static const BenchmarkMapEntry BenchmarkMap[] = {
    {BENCHMARK_TYPE_SEQUENTIAL_STRING, BenchmarkType_SEQUENTIAL},
    {BENCHMARK_TYPE_SEQUENTIAL_CACHED_STRING, BenchmarkType_SEQUENTIAL_CACHED},
    {BENCHMARK_TYPE_PARALLEL_STRING, BenchmarkType_PARALLEL},
    {BENCHMARK_TYPE_PARALLEL_CACHED_STRING, BenchmarkType_PARALLEL_CACHED},
};

#define BENCHMARK_MAP_COUNT (sizeof(BenchmarkMap) / sizeof(BenchmarkMap[0]))

TO_STRING_DECL(BenchmarkType) {
  const BenchmarkType BT = *(const BenchmarkType *)Self;
  const c8 *Result = "UNKNOWN";
  for (usize I = 0; I < BENCHMARK_MAP_COUNT; ++I) {
    if (BT == BenchmarkMap[I].Type) {
      Result = BenchmarkMap[I].AsString;
    }
  }
  return strdup(Result);
}

#define BENCHMARK_TYPE_MASK_DELIMITER " | "

TO_STRING_DECL(BenchmarkTypeMask) {
  const u32 Mask = *(const u32 *)Self;

  usize BufferSize = 1 + (BENCHMARK_MAP_COUNT - 1) *
                             (sizeof(BENCHMARK_TYPE_MASK_DELIMITER) - 1);
  for (usize I = 0; I < BENCHMARK_MAP_COUNT; ++I) {
    String S = TO_STRING(BenchmarkType)(&BenchmarkMap[I].Type);
    BufferSize += strlen(S);
  }
  c8 *Buffer = malloc(BufferSize);
  usize Length = 0;

  for (usize I = 0; I < BENCHMARK_MAP_COUNT; ++I) {
    if (Mask & BenchmarkMap[I].Type) {

      if (Length > 0) {
        Length += (usize)snprintf(Buffer + Length, BufferSize - Length,
                                  BENCHMARK_TYPE_MASK_DELIMITER);
      }
      Length += (usize)snprintf(Buffer + Length, BufferSize - Length, "%s",
                                BenchmarkMap[I].AsString);
    }
  }

  if (Length == 0) {
    snprintf(Buffer, BufferSize, "unknown");
  }

  return Buffer;
}

typedef struct ParseBenchmarkTypeMaskResult {
  u64 Value;
  bool Success;
} ParseBenchmarkTypeMaskResult;

static ParseBenchmarkTypeMaskResult ParseBenchmarkTypeMask(const c8 *Str) {
  ParseBenchmarkTypeMaskResult Result = {.Value = 0, .Success = false};

  MutableString StrCopy = strdup(Str);

  const c8 *Token = strtok(StrCopy, ",");
  while (Token) {
    for (size_t I = 0; I < BENCHMARK_MAP_COUNT; I++) {
      if (strcasecmp(Token, BenchmarkMap[I].AsString) == 0) {
        Result.Value |= BenchmarkMap[I].Type;
        Result.Success = true;
        break;
      }
    }

    if (!Result.Success) {
      return Result;
    }
    Token = strtok(NULLPTR, ",");
  }

  return Result;
}

NORETURN void Usage(const c8 *const ProgramName, const ExitStatus Status) {
  FILE *Out = (Status == ExitStatus_FAILURE ? stderr : stdout);
  fprintf(Out, "Usage: %s [options] <N>\n\n", ProgramName);
  fputs("N                              target value\n", Out);
  fputs("Options:\n", Out);
  fputs("    -h, -u, --help, --usage    display this message\n", Out);
  fputs("    -v, --version              display version\n", Out);

  exit((i32)Status);
}

CommandLineArgumentsParseResult
CommandLineArguments_Parse(CommandLineArguments *const Self, const i32 Argc,
                           const c8 *const *const Argv) {
  Self->Usage = false;
  Self->Version = false;
  Self->ProgramName = Argv[0];
  Self->N = 0;
  Self->TraceLevel = TraceLevel_NONE;
  Self->BenchmarkTypes = BenchmarkTypeMask_All;

  i32 OptionCharacter;
  i32 OptionIndex;
  while ((OptionCharacter = getopt_long(Argc, (c8 *const *)Argv, ShortOptions,
                                        LongOptions, &OptionIndex)) != -1) {
    switch (OptionCharacter) {
    case 'v': {
      Self->Version = true;
    } break;
    case 'h':
    case 'u': {
      Self->Usage = true;
    } break;
    case 't': {
      const ParseTraceLevelResult ParseResult = ParseTraceLevel(optarg);
      if (!ParseResult.Success) {
        fprintf(stderr, "Unknown trace level '%s', using NONE\n", optarg);
        Self->TraceLevel = TraceLevel_NONE;
      } else {
        Self->TraceLevel = ParseResult.Value;
      }
    } break;
    case 'r': {
      const ParseBenchmarkTypeMaskResult ParseResult =
          ParseBenchmarkTypeMask(optarg);
      if (!ParseResult.Success) {
        fprintf(stderr, "Unknown run type '%s', running all\n", optarg);
        Self->BenchmarkTypes = BenchmarkTypeMask_All;
      } else {
        Self->BenchmarkTypes = ParseResult.Value;
      }
    } break;
    case '?':
    default: {
      return CommandLineArgumentsParseResult_UNKNOWN_ARGUMENT;
    }
    }
  }

  if (Argv[optind] != NULL) {
    Self->N = strtoull(Argv[optind], NULLPTR, 10);
    const ParseUnsignedIntegerResult ParseResult =
        ParseUnsignedInteger(Argv[optind]);
    if (!ParseResult.Success) {
      return CommandLineArgumentsParseResult_INVALID_POSITIONAL_ARGUMENT;
    }
  } else {
    return CommandLineArgumentsParseResult_NO_POSITION_ARGUMENT;
  }

  return CommandLineArgumentsParseResult_SUCCESS;
}

TO_STRING_DEFINITION(
    CommandLineArguments, String BT = TO_STRING(BenchmarkTypeMask)(
                              &CommandLineArguments->BenchmarkTypes);
    String TL = TO_STRING(TraceLevel)(&CommandLineArguments->TraceLevel),
    "CommandLineArguments{BenchmarkTypes=%s, ProgramName=%s, N=%lu, "
    "TraceLevel=%s, Usage=%s, Version=%s}",
    BT, CommandLineArguments->ProgramName, CommandLineArguments->N, TL,
    btoa(CommandLineArguments->Usage), btoa(CommandLineArguments->Version));
