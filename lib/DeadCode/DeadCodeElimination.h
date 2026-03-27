#ifndef DEADCODEELIMINATION_H
#define DEADCODEELIMINATION_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class DeadCodeElimination : public PassInfoMixin<DeadCodeElimination> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // DEADCODEELIMINATION_H