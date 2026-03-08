#pragma once

#include <stdbool.h>

#include <common/types.h>
#include <vector/vector.h>

#include "cli.h"

typedef Vector(u64) Sequence;
typedef Vector(Sequence) AllSequences;

TO_STRING_DECL(Sequence);
TO_STRING_DECL(AllSequences);

/*
 * if TraceAllSequences then solution must fill AllSequences, after execution
 * moves into result
 */
typedef struct ExecutionOptions {
  AllSequences AllSequences;
  bool Trace;
} ExecutionOptions;

void ExecutionOptions_Init(ExecutionOptions *Self, TraceLevel TL, u64 N);

typedef struct Result {
  Sequence ResultSequence;
  AllSequences AllSequences;
  u64 MaxNumber;
} Result;

void Result_Destroy(Result *Self);
bool Result_Equals(const Result *Self, const Result *Other);
