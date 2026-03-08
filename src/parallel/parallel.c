#include "parallel/parallel.h"

#include <string.h>

#include <vector/vector.h>

static void WriteSequence(const AllSequences *const A, const Sequence *const C,
                          const usize CurrentNumber) {
  for (usize I = 0; I < Vector_Size(*C); ++I) {
    Vector_Push(Vector_At(*A, CurrentNumber - 1), Vector_At(*C, I));
  }
}

Result ParallelSolution(const u64 N, const ExecutionOptions Options) {
  u64 MaxNumber = 1;
  u64 MaxLength = 0;

  Sequence ResultSequence;
  if (Options.Trace) {
    Vector_Init(ResultSequence);
  }

#pragma omp parallel default(none)                                             \
    shared(N, MaxLength, MaxNumber, Options, ResultSequence)
  {
    u64 LocalMaxLength = 0;
    u64 LocalMaxNumber = 1;

    Sequence CurrentSequence = {0};
    Sequence LocalBestSequence = {0};

    if (Options.Trace) {
      Vector_Init(CurrentSequence);
      Vector_Init(LocalBestSequence);
    }

#pragma omp for schedule(static, 128)
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

      if (CurrentLength > LocalMaxLength) {
        LocalMaxLength = CurrentLength;
        LocalMaxNumber = CurrentNumber;

        if (Options.Trace) {
          Vector_Copy(LocalBestSequence, CurrentSequence);
        }
      }

      if (Options.Trace) {
        // for (usize i = 0; i < Vector_Size(CurrentSequence); ++i) {
        //   Vector_Push(Vector_At(Options.AllSequences, CurrentNumber - 1),
        //               Vector_At(CurrentSequence, i));
        // }
        WriteSequence(&Options.AllSequences, &CurrentSequence, CurrentNumber);
      }
    }

#pragma omp critical
    {
      if (LocalMaxLength > MaxLength) {
        MaxLength = LocalMaxLength;
        MaxNumber = LocalMaxNumber;

        if (Options.Trace) {
          Vector_Copy(ResultSequence, LocalBestSequence);
        }
      }
    }

    if (Options.Trace) {
      Vector_Destroy(CurrentSequence);
      Vector_Destroy(LocalBestSequence);
    }
  }

  Result R = {.MaxNumber = MaxNumber};

  memset(&R.ResultSequence, 0, sizeof(R.ResultSequence));
  memset(&R.AllSequences, 0, sizeof(R.AllSequences));

  if (Options.Trace) {
    memcpy(&R.ResultSequence, &ResultSequence, sizeof(R.ResultSequence));
    memcpy(&R.AllSequences, &Options.AllSequences, sizeof(R.AllSequences));
  }

  return R;
}

typedef Atomic u64 AtomicU64;

#if 0
Result ParallelSolutionCached(const u64 N, ExecutionOptions Options) {
  Vector(AtomicU64) Cache;
  Vector_InitWithCapacity(Cache, N);
  atomic_store_explicit(Vector_AtPtr(Cache, 0), 0, memory_order_relaxed);
  atomic_store_explicit(Vector_AtPtr(Cache, 1), 1, memory_order_relaxed);

  Vector_SizeUnsafe(Cache) += 2;

  u64 MaxLength = 1;
  u64 MaxNumber = 1;

  Sequence ResultSequence;
  if (Options.Trace) {
    Vector_Init(ResultSequence);
    Vector_Push(ResultSequence, 1);
    WriteSequence(&Options.AllSequences, &ResultSequence, 1);
  }

#pragma omp parallel default(none)                                             \
    shared(N, Cache, MaxLength, MaxNumber, Options, ResultSequence)
  {
    Sequence CurrentSequence;
    if (Options.Trace) {
      Vector_Init(CurrentSequence);
    }

    u64 LocalMaxLength = 1;
    u64 LocalMaxNumber = 1;
    Sequence LocalResultSequence;
    if (Options.Trace) {
      Vector_Init(LocalResultSequence);
    }

#pragma omp for schedule(static, 64) nowait
    for (u64 CurrentNumber = 2; CurrentNumber < N; ++CurrentNumber) {
      u64 CurrentValue = CurrentNumber;
      u64 CurrentLength = 0;

      if (Options.Trace) {
        Vector_Clear(CurrentSequence);
        Vector_Push(CurrentSequence, CurrentValue);
      }

      while (CurrentValue != 1 &&
             (CurrentValue >= N ||
              atomic_load_explicit(Vector_AtPtr(Cache, CurrentValue),
                                   memory_order_relaxed) == 0)) {
        CurrentValue =
            (CurrentValue % 2 == 0) ? CurrentValue / 2 : 3 * CurrentValue + 1;
        CurrentLength++;

        if (Options.Trace) {
          Vector_Push(CurrentSequence, CurrentValue);
        }
      }

      if (CurrentValue < N) {
        CurrentLength += atomic_load_explicit(Vector_AtPtr(Cache, CurrentValue),
                                              memory_order_relaxed);
      }

      if (Options.Trace) {
        const Sequence CachedSequence =
            Vector_At(Options.AllSequences, CurrentValue - 1);
        for (u64 i = 1; i < Vector_Size(CachedSequence); ++i)
          Vector_Push(CurrentSequence, Vector_At(CachedSequence, i));

        WriteSequence(&Options.AllSequences, &CurrentSequence, CurrentNumber);
      }

      if (CurrentNumber < N) {
        atomic_store_explicit(Vector_AtPtr(Cache, CurrentNumber), CurrentLength,
                              memory_order_relaxed);
      }

      if (CurrentLength > LocalMaxLength) {
        LocalMaxLength = CurrentLength;
        LocalMaxNumber = CurrentNumber;
        if (Options.Trace) {
          Vector_Copy(LocalResultSequence, CurrentSequence);
        }
      }
    }

#pragma omp critical
    {
      if (LocalMaxLength > MaxLength) {
        MaxLength = LocalMaxLength;
        MaxNumber = LocalMaxNumber;
        if (Options.Trace) {
          Vector_Copy(ResultSequence, LocalResultSequence);
        }
      }
    }

    if (Options.Trace) {
      Vector_Destroy(CurrentSequence);
      Vector_Destroy(LocalResultSequence);
    }
  }

  Vector_Destroy(Cache);

  Result R = {.MaxNumber = MaxNumber};
  if (Options.Trace) {
    memcpy(&R.ResultSequence, &ResultSequence, sizeof(R.ResultSequence));
    memcpy(&R.AllSequences, &Options.AllSequences, sizeof(R.AllSequences));
  }

  return R;
}
#endif

Result ParallelSolutionCached(const u64 N, ExecutionOptions Options) {
  // --- Инициализация атомарного кеша ---
  Vector(AtomicU64) Cache;
  Vector_InitWithCapacity(Cache, N);
  atomic_store_explicit(Vector_AtPtr(Cache, 0), 0, memory_order_relaxed);
  atomic_store_explicit(Vector_AtPtr(Cache, 1), 1, memory_order_relaxed);
  Vector_SizeUnsafe(Cache) += 2;

  u64 MaxLength = 1;
  u64 MaxNumber = 1;

  Sequence ResultSequence;
  if (Options.Trace) {
    Vector_Init(ResultSequence);
    Vector_Push(ResultSequence, 1);
    WriteSequence(&Options.AllSequences, &ResultSequence, 1);
  }

#pragma omp parallel default(none)                                             \
    shared(N, Cache, MaxLength, MaxNumber, Options, ResultSequence)
  {
    Sequence CurrentSequence;
    if (Options.Trace)
      Vector_Init(CurrentSequence);

    u64 LocalMaxLength = 1;
    u64 LocalMaxNumber = 1;
    Sequence LocalResultSequence = {0};
    if (Options.Trace)
      Vector_Init(LocalResultSequence);

#pragma omp for schedule(static, 128)
    for (u64 CurrentNumber = 2; CurrentNumber < N; ++CurrentNumber) {
      u64 CurrentValue = CurrentNumber;
      u64 CurrentLength = 0;

      if (Options.Trace) {
        Vector_Clear(CurrentSequence);
        Vector_Push(CurrentSequence, CurrentValue);
      }

      // --- Основной цикл Collatz с настоящим спинлоком на кеш ---
      u64 cached_length = 0;
      while (CurrentValue != 1) {
        if (CurrentValue < N) {
          // Спинлок: ждем, пока другой поток вычислит Cache
          cached_length = atomic_load_explicit(
              Vector_AtPtr(Cache, CurrentValue), memory_order_acquire);
          if (cached_length != 0)
            break; // готово
        }

        // Иначе продолжаем вычислять Collatz
        CurrentValue =
            (CurrentValue % 2 == 0) ? CurrentValue / 2 : 3 * CurrentValue + 1;
        CurrentLength++;

        if (Options.Trace) {
          Vector_Push(CurrentSequence, CurrentValue);
        }
      }

      // Если CurrentValue < N, добавляем длину из кеша
      if (CurrentValue < N) {
        CurrentLength += cached_length;

        if (Options.Trace) {
          const Sequence CachedSequence =
              Vector_At(Options.AllSequences, CurrentValue - 1);
          for (u64 i = 1; i < Vector_Size(CachedSequence); ++i)
            Vector_Push(CurrentSequence, Vector_At(CachedSequence, i));
        }
      }

      if (Options.Trace) {
        WriteSequence(&Options.AllSequences, &CurrentSequence, CurrentNumber);
      }

      if (CurrentNumber < N) {
        atomic_store_explicit(Vector_AtPtr(Cache, CurrentNumber), CurrentLength,
                              memory_order_release);
      }

      if (CurrentLength > LocalMaxLength) {
        LocalMaxLength = CurrentLength;
        LocalMaxNumber = CurrentNumber;
        if (Options.Trace) {
          Vector_Copy(LocalResultSequence, CurrentSequence);
        }
      }
    } // for CurrentNumber

#pragma omp critical
    {
      if (LocalMaxLength > MaxLength) {
        MaxLength = LocalMaxLength;
        MaxNumber = LocalMaxNumber;
        if (Options.Trace) {
          Vector_Copy(ResultSequence, LocalResultSequence);
        }
      }
    }

    if (Options.Trace) {
      Vector_Destroy(CurrentSequence);
      Vector_Destroy(LocalResultSequence);
    }
  } // parallel

  Vector_Destroy(Cache);

  Result R = {.MaxNumber = MaxNumber};
  if (Options.Trace) {
    memcpy(&R.ResultSequence, &ResultSequence, sizeof(R.ResultSequence));
    memcpy(&R.AllSequences, &Options.AllSequences, sizeof(R.AllSequences));
  }

  return R;
}
