#include "execution.h"

#include <string.h>

#include <string/to_string.h>

#define COLLATZ_SEQUENCE_DELIMITER " -> "

TO_STRING_DECL(Sequence) {
  const Sequence S = *(const Sequence *)Self;
  const usize Size = Vector_Size(S);

  usize TotalBufferSize = 1; // null-terminator
  for (usize i = 0; i < Size; ++i) {
    c8 CurrentElementBuffer[32];
    snprintf(CurrentElementBuffer, sizeof(CurrentElementBuffer), "%lu",
             Vector_At(S, i));
    TotalBufferSize += strlen(CurrentElementBuffer);
    if (i + 1 < Size)
      TotalBufferSize += sizeof(COLLATZ_SEQUENCE_DELIMITER) - 1;
  }

  c8 *Buffer = malloc(TotalBufferSize);
  usize Length = 0;

  for (usize i = 0; i < Size; ++i) {
    if (i > 0) {
      Length += (usize)snprintf(Buffer + Length, TotalBufferSize - Length, "%s",
                                COLLATZ_SEQUENCE_DELIMITER);
    }
    Length += (usize)snprintf(Buffer + Length, TotalBufferSize - Length, "%lu",
                              Vector_At(S, i));
  }

  Buffer[Length] = '\0';
  return Buffer;
}

TO_STRING_DECL(AllSequences) {
  const AllSequences AS = *(const AllSequences *)Self;

  const usize Count = Vector_Size(AS);

  const c8 **const SequencesAsStrings = malloc(sizeof(c8 *) * Count);
  usize TotalBufferSize = 1; // null-terminator

  for (usize I = 0; I < Count; ++I) {
    Sequence Seq = Vector_At(AS, I);
    SequencesAsStrings[I] = TO_STRING(Sequence)(&Seq);
    char IndexBuffer[32];
    snprintf(IndexBuffer, sizeof(IndexBuffer), "[%zu] : ", I + 1);
    TotalBufferSize +=
        strlen(IndexBuffer) + strlen(SequencesAsStrings[I]) + 1; // newline
  }

  c8 *Buffer = malloc(TotalBufferSize);
  usize Length = 0;

  for (usize I = 0; I < Count; ++I) {
    if (I > 0)
      Buffer[Length++] = '\n';

    Length += (usize)snprintf(Buffer + Length, TotalBufferSize - Length,
                              "[%zu] : ", I + 1);
    Length += (usize)snprintf(Buffer + Length, TotalBufferSize - Length, "%s",
                              SequencesAsStrings[I]);
    free((void *)SequencesAsStrings[I]);
  }

  free(SequencesAsStrings);
  Buffer[Length] = '\0';
  return Buffer;
}

void ExecutionOptions_Init(ExecutionOptions *Self, const TraceLevel TL,
                           const u64 N) {
  Self->Trace = false;

  switch (TL) {
  case TraceLevel_ALL:
  case TraceLevel_RESULT:
    Self->Trace = true;
    FALLTHROUGH
  case TraceLevel_NONE:
    break;
  }
  if (Self->Trace) {
    Vector_InitWithCapacity(Self->AllSequences, N - 1);
    for (usize Index = 0; Index + 1 < N; ++Index) {
      Sequence Tmp;
      Vector_Init(Tmp); // TODO: calculate approx length
      Vector_Push(Self->AllSequences, Tmp);
    }
  }
}

void Result_Destroy(Result *Self) {
  Vector_Destroy(Self->ResultSequence);
  for (usize Index = 0; Index < Vector_Size(Self->AllSequences); ++Index) {
    Vector_Destroy(Vector_At(Self->AllSequences, Index));
  }
  Vector_Destroy(Self->AllSequences);
}

static bool Sequence_Equals(const Sequence *const Self,
                            const Sequence *const Other) {
  if (Vector_Size(*Self) != Vector_Size(*Other)) {
    return false;
  }

  for (usize I = 0; I < Vector_Size(*Self); ++I) {
    if (Vector_At(*Self, I) != Vector_At(*Other, I)) {
      return false;
    }
  }
  return true;
}

bool Result_Equals(const Result *const Self, const Result *const Other) {
  if (Self->MaxNumber != Other->MaxNumber) {
    return false;
  }

  if (!Sequence_Equals(&Self->ResultSequence, &Other->ResultSequence) ||
      Vector_Size(Self->AllSequences) != Vector_Size(Other->AllSequences)) {
    return false;
  }

  for (usize I = 0; I < Vector_Size(Self->AllSequences); ++I) {
    const Sequence *const SelfCurrent = Vector_AtPtr(Self->AllSequences, I);
    const Sequence *const OtherCurrent = Vector_AtPtr(Other->AllSequences, I);
    if (!Sequence_Equals(SelfCurrent, OtherCurrent)) {
      return false;
    }
  }

  return true;
}
