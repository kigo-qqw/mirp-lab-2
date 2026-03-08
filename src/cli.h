#pragma once

#include "vector/vector.h"

#include <stdbool.h>

#include <common/types.h>
#include <string/to_string.h>

typedef enum ExitStatus {
  ExitStatus_SUCCESS = 0,
  ExitStatus_FAILURE = 1,
} ExitStatus;

typedef enum CommandLineArgumentsParseResult {
  // TODO: parse errors, invalid argument, unknown argument, etc.
  CommandLineArgumentsParseResult_INVALID_POSITIONAL_ARGUMENT = -3,
  CommandLineArgumentsParseResult_NO_POSITION_ARGUMENT = -2,
  CommandLineArgumentsParseResult_UNKNOWN_ARGUMENT = -1,
  CommandLineArgumentsParseResult_SUCCESS = 0,
} CommandLineArgumentsParseResult;

typedef enum TraceLevel {
  TraceLevel_NONE = 0,
  TraceLevel_RESULT = 1,
  TraceLevel_ALL = 2,
} TraceLevel;

TO_STRING_DECL(TraceLevel);

typedef enum BenchmarkType {
  BenchmarkType_SEQUENTIAL = 1 << 0,
  BenchmarkType_SEQUENTIAL_CACHED = 1 << 1,
  BenchmarkType_PARALLEL = 1 << 2,
  BenchmarkType_PARALLEL_CACHED = 1 << 3
} BenchmarkType;

typedef u64 BenchmarkTypeMask;

enum {
  BenchmarkTypeMask_All = BenchmarkType_SEQUENTIAL |
                          BenchmarkType_SEQUENTIAL_CACHED |
                          BenchmarkType_PARALLEL | BenchmarkType_PARALLEL_CACHED
};

TO_STRING_DECL(BenchmarkType);
TO_STRING_DECL(BenchmarkTypeMask);

typedef struct CommandLineArguments {
  const c8 *ProgramName;
  BenchmarkTypeMask BenchmarkTypes;
  u64 N;
  TraceLevel TraceLevel;
  bool Usage;
  bool Version;
} CommandLineArguments;

void Usage(const c8 *ProgramName, ExitStatus Status);

CommandLineArgumentsParseResult
CommandLineArguments_Parse(CommandLineArguments *Self, i32 Argc,
                           const c8 *const *Argv);

TO_STRING_DECL(CommandLineArguments);
