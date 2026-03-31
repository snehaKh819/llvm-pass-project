#include "DeadCodeElimination.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SetVector.h"

using namespace llvm;

#define DEBUG_TYPE "dce"

char DeadCodeElimination::ID = 0;

// Register the pass with LLVM
static RegisterPass<DeadCodeElimination> X("my-dce", 
                                           "Custom Dead Code Elimination Pass",
                                           false, false);

// Helper: Check if an instruction is trivially dead and process it
bool DeadCodeElimination::DCEInstruction(Instruction *I, 
                                          SmallSetVector<Instruction*, 16> &WorkList) {
  // Check if this instruction is trivially dead
  if (isInstructionTriviallyDead(I)) {
    LLVM_DEBUG(dbgs() << "DCE: Removing dead instruction: " << *I << "\n");
    
    // Preserve debug info before deletion
    salvageDebugInfo(*I);
    
    // For each operand, null it out and check if it becomes dead
    for (Use &U : I->operands()) {
      Instruction *OpInst = dyn_cast<Instruction>(U.get());
      if (OpInst) {
        // Null out the operand
        U.set(nullptr);
        
        // If the operand instruction has no uses and is trivially dead,
        // add it to the worklist for potential removal
        if (OpInst->use_empty() && isInstructionTriviallyDead(OpInst)) {
          WorkList.insert(OpInst);
        }
      }
    }
    
    // Remove the instruction
    I->eraseFromParent();
    return true;
  }
  return false;
}

// Main elimination driver using worklist algorithm
bool DeadCodeElimination::eliminateDeadCode(Function &F) {
  bool Changed = false;
  SmallSetVector<Instruction*, 16> WorkList;
  
  // First pass: Collect all potentially dead instructions
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (isInstructionTriviallyDead(&I)) {
        WorkList.insert(&I);
      }
    }
  }
  
  // Process worklist until empty
  while (!WorkList.empty()) {
    Instruction *I = WorkList.pop_back_val();
    
    // Process this instruction (it may have been processed already)
    if (DCEInstruction(I, WorkList)) {
      Changed = true;
    }
  }
  
  return Changed;
}

// Main entry point for the pass
bool DeadCodeElimination::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "Running DCE on function: " << F.getName() << "\n");
  
  bool Changed = eliminateDeadCode(F);
  
  if (Changed) {
    LLVM_DEBUG(dbgs() << "DCE: Made changes to function: " << F.getName() << "\n");
  }
  
  return Changed;
}