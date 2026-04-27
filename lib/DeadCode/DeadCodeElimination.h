#ifndef DEADCODEELIMINATION_H
#define DEADCODEELIMINATION_H

#include "llvm/Pass.h"
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

class DeadCodeElimination : public FunctionPass {
public:
  static char ID;
  DeadCodeElimination() : FunctionPass(ID) {}
  
  bool runOnFunction(Function &F) override;
  
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