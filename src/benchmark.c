#include "benchmark.h"

#include <time.h>

struct timespec GetTime(void) {
  struct timespec Spec;
  clock_gettime(CLOCK_MONOTONIC, &Spec);
  return Spec;
}

enum { MICROSECOND_AS_NANOSECOND = 1000 };
enum { SECOND_AS_MILLISECOND = 1000 };
enum { MILLISECOND_AS_MICROSECOND = 1000 };
enum {
  SECOND_AS_MICROSECOND =
      (u64)SECOND_AS_MILLISECOND * (u64)MILLISECOND_AS_MICROSECOND
};
enum {
  SECOND_AS_NANOSECOND =
      (u64)SECOND_AS_MICROSECOND * (u64)MICROSECOND_AS_NANOSECOND
};

f64 TimeDiffInSeconds(const struct timespec Start, const struct timespec End) {
  struct timespec Tmp;
  if (End.tv_nsec - Start.tv_nsec < 0) {
    Tmp.tv_sec = End.tv_sec - Start.tv_sec - 1;
    Tmp.tv_nsec = SECOND_AS_NANOSECOND + End.tv_nsec - Start.tv_nsec;
  } else {
    Tmp.tv_sec = End.tv_sec - Start.tv_sec;
    Tmp.tv_nsec = End.tv_nsec - Start.tv_nsec;
  }
  const f64 Microseconds = (f64)Tmp.tv_sec * (f64)SECOND_AS_MICROSECOND +
                           (f64)Tmp.tv_nsec / (f64)MICROSECOND_AS_NANOSECOND;
  return Microseconds / (f64)SECOND_AS_MICROSECOND;
}

Result Benchmark_Run(const Benchmark *Self) {

  ExecutionOptions Options;
  ExecutionOptions_Init(&Options, Self->TraceLevel, Self->N);

  const struct timespec Start = GetTime();
  const Result R = Self->RunFunction(Self->N, Options);
  const struct timespec End = GetTime();

  printf("[%s] Time : %lfs\n", Self->Name, TimeDiffInSeconds(Start, End));
  printf("[%s] Maximum Number: %lu\n", Self->Name, R.MaxNumber);

  switch (Self->TraceLevel) {
  case TraceLevel_ALL: {
    String AllSequencesAsString = TO_STRING(AllSequences)(&R.AllSequences);
    printf("[%s] : Trace of all numbers:\n%s\n", Self->Name,
           AllSequencesAsString);
  }
    FALLTHROUGH
  case TraceLevel_RESULT: {
    String ResultSequenceAsString = TO_STRING(Sequence)(&R.ResultSequence);
    printf("[%s] : Longest Collatz sequence: %s\n", Self->Name,
           ResultSequenceAsString);
  }
    FALLTHROUGH
  case TraceLevel_NONE:
    break;
  }

  return R;
}
