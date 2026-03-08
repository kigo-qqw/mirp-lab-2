#pragma once

#include <time.h>

#include <common/types.h>

#include "execution.h"

struct timespec GetTime(void);

f64 TimeDiffInSeconds(struct timespec Start, struct timespec End);

typedef Result (*RunFunction)(u64, ExecutionOptions);

typedef struct Benchmark {
  const c8 *Name;
  RunFunction RunFunction;
  u64 N;
  TraceLevel TraceLevel;
} Benchmark;

Result Benchmark_Run(const Benchmark *Self);
