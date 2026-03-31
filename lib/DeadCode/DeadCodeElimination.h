#ifndef DEADCODEELIMINATION_H
#define DEADCODEELIMINATION_H

#include "llvm/Pass.h"
#include "llvm/ADT/SetVector.h"

namespace llvm {
  class Function;
  class Instruction;
}

using namespace llvm;

class DeadCodeElimination : public FunctionPass {
public:
  static char ID;
  DeadCodeElimination() : FunctionPass(ID) {}
  
  bool runOnFunction(Function &F) override;
  
private:
  // Helper function to process a single instruction
  bool DCEInstruction(Instruction *I, 
                      SmallSetVector<Instruction*, 16> &WorkList);
  
  // Main elimination driver
  bool eliminateDeadCode(Function &F);
};

#endif