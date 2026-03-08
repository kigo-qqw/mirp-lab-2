#pragma once

#include <common/types.h>

#include "execution.h"

Result ParallelSolution(const u64 N, const ExecutionOptions Options);

Result ParallelSolutionCached(const u64 N, const ExecutionOptions Options);
