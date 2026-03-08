#include "benchmark.h"

#include <common/types.h>

#include "cli.h"
#include "execution.h"
#include "parallel/parallel.h"
#include "sequential/sequential.h"

#ifndef VERSION
#error Provide VERSION as -DVERSION="<version>"
#endif

#ifndef PROJECT_NAME
#error Provide PROJECT_NAME as -DPROJECT_NAME="<project_name>"
#endif

typedef struct BenchmarkTypeToRunFunctionMapEntry {
  const BenchmarkType Type;
  const RunFunction RunFunction;
} BenchmarkTypeToRunFunctionMapEntry;

static const BenchmarkTypeToRunFunctionMapEntry
    BenchmarkTypeToRunFunctionMap[] = {
        {BenchmarkType_SEQUENTIAL, SequentialSolution},
        {BenchmarkType_SEQUENTIAL_CACHED, SequentialSolutionCached},
        {BenchmarkType_PARALLEL, ParallelSolution},
        {BenchmarkType_PARALLEL_CACHED, ParallelSolutionCached}};
enum {
  BENCHMARK_TYPE_TO_RUN_FUNCTION_MAP_COUNT =
      (sizeof(BenchmarkTypeToRunFunctionMap) /
       sizeof(BenchmarkTypeToRunFunctionMap[0]))
};

i32 main(const i32 Argc, const c8 *const *const Argv) {
  CommandLineArguments Args;
  const CommandLineArgumentsParseResult CliParseResult =
      CommandLineArguments_Parse(&Args, Argc, Argv);

#ifndef NDEBUG
  String ArgsAsString = TO_STRING(CommandLineArguments)(&Args);
  puts(ArgsAsString);
#endif

  if (Args.Usage) {
    Usage(Args.ProgramName, ExitStatus_SUCCESS);
  }
  if (Args.Version) {
    printf(PROJECT_NAME " %s\n", VERSION);
    exit(ExitStatus_SUCCESS);
  }
  if (CliParseResult != CommandLineArgumentsParseResult_SUCCESS) {
    Usage(Args.ProgramName, ExitStatus_FAILURE);
  }

  Result PreviousResult = {0};

  for (usize I = 0; I < BENCHMARK_TYPE_TO_RUN_FUNCTION_MAP_COUNT; ++I) {
    if (Args.BenchmarkTypes & BenchmarkTypeToRunFunctionMap[I].Type) {
      String Name =
          TO_STRING(BenchmarkType)(&BenchmarkTypeToRunFunctionMap[I].Type);
      Result CurrentResult = Benchmark_Run(&(Benchmark){
          .Name = Name,
          .RunFunction = BenchmarkTypeToRunFunctionMap[I].RunFunction,
          .N = Args.N,
          .TraceLevel = Args.TraceLevel});
      if (PreviousResult.MaxNumber != 0 &&
          !Result_Equals(&PreviousResult, &CurrentResult)) {
        fprintf(stderr, "[ERROR] : Results unequal\n");
      }
      Result_Destroy(&PreviousResult);
      PreviousResult = CurrentResult;
    }
  }
  Result_Destroy(&PreviousResult);

  return ExitStatus_SUCCESS;
}
