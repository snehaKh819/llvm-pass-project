#ifndef DEADCODEELIMINATION_H
#define DEADCODEELIMINATION_H

#include "llvm/IR/PassManager.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SetVector.h"
#include <vector>

namespace llvm {
  class Function;
  class Instruction;
  class BasicBlock;
  class BranchInst;
  class AllocaInst;
  class StoreInst;
  class LoadInst;
}

using namespace llvm;

class DeadCodeElimination : public PassInfoMixin<DeadCodeElimination> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  
private:
  // Stage 1: Trivial dead instructions
  bool DCEInstruction(Instruction *I, 
                      SmallSetVector<Instruction*, 16> &WorkList);
  bool eliminateDeadCode(Function &F);
  
  // Stage 2: Dead control flow
  bool eliminateSameTargetBranches(Function &F);
  bool eliminateForwardingBlocks(Function &F);
  bool simplifyControlFlow(Function &F);
  void updatePHINodesForRemovedBlock(BasicBlock *RemovedBlock, 
                                      BasicBlock *TargetBlock,
                                      const std::vector<BasicBlock*> &Predecessors);
  
  // Stage 3: Dead stack slots
  bool eliminateDeadStackSlots(Function &F);
  bool isAllocaDead(AllocaInst *AI);
  void removeDeadStores(AllocaInst *AI);
};

#endif