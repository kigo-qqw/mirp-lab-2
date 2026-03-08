#pragma once

#include <common/types.h>

#include "execution.h"

Result SequentialSolution(u64 N, ExecutionOptions Options);

Result SequentialSolutionCached(u64 N, ExecutionOptions Options);
