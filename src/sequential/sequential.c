#include "sequential/sequential.h"

#include <string.h>
#include <vector/vector.h>

Result SequentialSolution(const u64 N, const ExecutionOptions Options) {
  u64 MaxLength = 0;
  u64 MaxNumber = 1;

  Sequence ResultSequence = {0};
  Sequence CurrentSequence = {0};
  if (Options.Trace) {
    Vector_Init(ResultSequence);
    Vector_Init(CurrentSequence);
  }

  for (usize CurrentNumber = 1; CurrentNumber < N; ++CurrentNumber) {
    u64 CurrentValue = CurrentNumber;
    u64 CurrentLength = 1;

    if (Options.Trace) {
      Vector_Clear(CurrentSequence);
      Vector_Push(CurrentSequence, CurrentValue);
    }

    while (CurrentValue != 1) {
      CurrentValue =
          CurrentValue % 2 == 0 ? CurrentValue / 2 : 3 * CurrentValue + 1;
      ++CurrentLength;

      if (Options.Trace) {
        Vector_Push(CurrentSequence, CurrentValue);
      }
    }

    if (CurrentLength > MaxLength) {
      MaxLength = CurrentLength;
      MaxNumber = CurrentNumber;

      if (Options.Trace) {
        Vector_Copy(ResultSequence, CurrentSequence);
      }
    }

    if (Options.Trace) {
      for (usize i = 0; i < Vector_Size(CurrentSequence); ++i) {
        Vector_Push(Vector_At(Options.AllSequences, CurrentNumber - 1),
                    Vector_At(CurrentSequence, i));
      }
    }
  }

  Result R = {.MaxNumber = MaxNumber};

  memset(&R.ResultSequence, 0, sizeof(R.ResultSequence));
  memset(&R.AllSequences, 0, sizeof(R.AllSequences));

  if (Options.Trace) {
    memcpy(&R.ResultSequence, &ResultSequence, sizeof(R.ResultSequence));
    Vector_Destroy(CurrentSequence);
    memcpy(&R.AllSequences, &Options.AllSequences, sizeof(R.AllSequences));
  }

  return R;
}

Result SequentialSolutionCached(const u64 N, ExecutionOptions Options) {
  Vector(u64) Cache;
  Vector_InitWithCapacity(Cache, N);
  Vector_Push(Cache, 0);

  u64 MaxLength = 0;
  u64 MaxNumber = 0;

  Sequence ResultSequence = {0};
  Sequence CurrentSequence = {0};
  if (Options.Trace) {
    Vector_Init(ResultSequence);
    Vector_Init(CurrentSequence);
  }

  for (u64 CurrentNumber = 1; CurrentNumber < N; CurrentNumber++) {
    u64 CurrentValue = CurrentNumber;
    u64 CurrentLength = 0;

    if (Options.Trace) {
      Vector_Clear(CurrentSequence);
      Vector_Push(CurrentSequence, CurrentValue);
    }

    while (CurrentValue != 1 &&
           (CurrentValue >= N || Vector_At(Cache, CurrentValue) == 0)) {
      CurrentValue =
          CurrentValue % 2 == 0 ? CurrentValue / 2 : 3 * CurrentValue + 1;
      CurrentLength++;

      if (Options.Trace) {
        Vector_Push(CurrentSequence, CurrentValue);
      }
    }

    CurrentLength += Vector_At(Cache, CurrentValue);

    if (Options.Trace) {
      const Sequence CachedSequence =
          Vector_At(Options.AllSequences, CurrentValue - 1);
      for (u64 I = 1; I < Vector_Size(CachedSequence); ++I) {
        Vector_Push(CurrentSequence, Vector_At(CachedSequence, I));
      }

      for (usize i = 0; i < Vector_Size(CurrentSequence); ++i) {
        Vector_Push(Vector_At(Options.AllSequences, CurrentNumber - 1),
                    Vector_At(CurrentSequence, i));
      }
    }

    Vector_Push(Cache, CurrentLength);

    if (CurrentLength > MaxLength) {
      MaxLength = CurrentLength;
      MaxNumber = CurrentNumber;

      if (Options.Trace) {
        Vector_Copy(ResultSequence, CurrentSequence);
      }
    }
  }

  Vector_Destroy(Cache);

  Result R = {.MaxNumber = MaxNumber};

  if (Options.Trace) {
    memcpy(&R.ResultSequence, &ResultSequence, sizeof(R.ResultSequence));
    Vector_Destroy(CurrentSequence);
    memcpy(&R.AllSequences, &Options.AllSequences, sizeof(R.AllSequences));
  }

  return R;
}
