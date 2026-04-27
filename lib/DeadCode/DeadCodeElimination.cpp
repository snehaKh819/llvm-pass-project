#include "DeadCodeElimination.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

#define DEBUG_TYPE "dce"

char DeadCodeElimination::ID = 0;



static RegisterPass<DeadCodeElimination> X("my-dce", 
                                           "Custom Dead Code Elimination Pass",
                                           false, false);



bool DeadCodeElimination::DCEInstruction(Instruction *I, 
                                          SmallSetVector<Instruction*, 16> &WorkList) {
  if (isInstructionTriviallyDead(I)) {
    LLVM_DEBUG(dbgs() << "DCE: Removing dead instruction: " << *I << "\n");
    
    salvageDebugInfo(*I);
    
    for (Use &U : I->operands()) {
      Instruction *OpInst = dyn_cast<Instruction>(U.get());
      if (OpInst) {
        U.set(nullptr);
        
    
        if (OpInst->use_empty() && isInstructionTriviallyDead(OpInst)) {
          WorkList.insert(OpInst);
        }
      }
    }
    
    
    I->eraseFromParent();
    return true;
  }
  return false;
}


bool DeadCodeElimination::eliminateDeadCode(Function &F) {
  bool Changed = false;
  SmallSetVector<Instruction*, 16> WorkList;
  
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (isInstructionTriviallyDead(&I)) {
        WorkList.insert(&I);
      }
    }
  }
 
  while (!WorkList.empty()) {
    Instruction *I = WorkList.pop_back_val();
    
    if (DCEInstruction(I, WorkList)) {
      Changed = true;
    }
  }
  
  return Changed;
}

// Stage 2: Dead Control Flow

bool DeadCodeElimination::eliminateSameTargetBranches(Function &F) {
  bool Changed = false;
  
  for (BasicBlock &BB : F) {
    Instruction *Term = BB.getTerminator();
    
    BranchInst *BI = dyn_cast<BranchInst>(Term);
    if (!BI || !BI->isConditional()) {
      continue;
    }
    
    BasicBlock *TrueTarget = BI->getSuccessor(0);
    BasicBlock *FalseTarget = BI->getSuccessor(1);
    
    if (TrueTarget == FalseTarget) {
      LLVM_DEBUG(dbgs() << "CFG: Found same-target branch in block " 
                        << BB.getName() << "\n");
      
      BranchInst::Create(TrueTarget, &BB);
      
      BI->eraseFromParent();
      
      Changed = true;
      LLVM_DEBUG(dbgs() << "CFG: Replaced with unconditional branch\n");
    }
  }
  
  return Changed;
}

void DeadCodeElimination::updatePHINodesForRemovedBlock(
    BasicBlock *RemovedBlock,
    BasicBlock *TargetBlock,
    const std::vector<BasicBlock*> &Predecessors) {
  
  for (PHINode &PN : TargetBlock->phis()) {
    Value *IncomingValue = PN.getIncomingValueForBlock(RemovedBlock);
    
    PN.removeIncomingValue(RemovedBlock);
    
    for (BasicBlock *Pred : Predecessors) {
      PN.addIncoming(IncomingValue, Pred);
    }
  }
}

bool DeadCodeElimination::eliminateForwardingBlocks(Function &F) {
  bool Changed = false;
  bool LocalChanged = true;
  
  while (LocalChanged) {
    LocalChanged = false;
    
    for (auto It = F.begin(); It != F.end(); ) {
      BasicBlock *BB = &*It;
      ++It;
      
      
      if (&F.getEntryBlock() == BB) {
        continue;
      }
      
    
      unsigned NonPHICount = 0;
      Instruction *BranchInstPtr = nullptr;
      
      for (Instruction &I : *BB) {
        if (!isa<PHINode>(&I)) {
          NonPHICount++;
          if (NonPHICount == 1) {
            BranchInstPtr = &I;
          } else {
           
            break;
          }
        }
      }
      
      if (NonPHICount != 1) {
        continue;
      }
      
      BranchInst *BI = dyn_cast<BranchInst>(BranchInstPtr);
      if (!BI || BI->isConditional()) {
        continue;
      }
      
      BasicBlock *TargetBlock = BI->getSuccessor(0);
      
      if (TargetBlock == BB) {
        continue;
      }
      
      LLVM_DEBUG(dbgs() << "CFG: Found forwarding block: " << BB->getName() 
                        << " -> " << TargetBlock->getName() << "\n");
      
     
      std::vector<BasicBlock*> Predecessors;
      for (BasicBlock *Pred : predecessors(BB)) {
        Predecessors.push_back(Pred);
      }
      
     
      updatePHINodesForRemovedBlock(BB, TargetBlock, Predecessors);
      
    
      for (BasicBlock *Pred : Predecessors) {
        Instruction *PredTerm = Pred->getTerminator();
        
        
        if (BranchInst *PredBI = dyn_cast<BranchInst>(PredTerm)) {
          if (PredBI->isConditional()) {
          
            if (PredBI->getSuccessor(0) == BB) {
              PredBI->setSuccessor(0, TargetBlock);
            }
            if (PredBI->getSuccessor(1) == BB) {
              PredBI->setSuccessor(1, TargetBlock);
            }
          } else {
           
            PredBI->setSuccessor(0, TargetBlock);
          }
        }

      }
      
      BB->eraseFromParent();
      LocalChanged = true;
      Changed = true;
      LLVM_DEBUG(dbgs() << "CFG: Removed forwarding block\n");
    }
  }
  
  return Changed;
}


bool DeadCodeElimination::simplifyControlFlow(Function &F) {
  bool Changed = false;
  bool LocalChanged = true;
  
  
  while (LocalChanged) {
    LocalChanged = false;
    
    
    if (eliminateSameTargetBranches(F)) {
      LocalChanged = true;
      Changed = true;
    }
    
    if (eliminateForwardingBlocks(F)) {
      LocalChanged = true;
      Changed = true;
    }
  }
  
  return Changed;
}


// Stage 3: Dead Stack Slots (FIXED VERSION)


// Check if an alloca instruction has any loads (if not, it's dead)
bool DeadCodeElimination::isAllocaDead(AllocaInst *AI) {
  // Track if we find any load instruction using this alloca
  bool hasLoad = false;
  
  // Iterate over all users of this alloca
  for (User *U : AI->users()) {
    // Check if the user is a load instruction
    if (LoadInst *LI = dyn_cast<LoadInst>(U)) {
      hasLoad = true;
      LLVM_DEBUG(dbgs() << "Stack: Alloca " << AI->getName() 
                        << " has load: " << *LI << "\n");
      break;  // Found a load, alloca is live
    }
  }
  
  // Alloca is dead if it has no loads
  return !hasLoad;
}

// Remove all stores to an alloca that will never be loaded
void DeadCodeElimination::removeDeadStores(AllocaInst *AI) {
  // Collect all store instructions using this alloca
  SmallVector<StoreInst*, 16> DeadStores;
  
  for (User *U : AI->users()) {
    if (StoreInst *SI = dyn_cast<StoreInst>(U)) {
      // Check if this store's value is stored to this alloca
      if (SI->getPointerOperand() == AI) {
        DeadStores.push_back(SI);
        LLVM_DEBUG(dbgs() << "Stack: Found dead store: " << *SI << "\n");
      }
    }
  }
  
  // Remove all dead stores
  for (StoreInst *SI : DeadStores) {
    LLVM_DEBUG(dbgs() << "Stack: Removing dead store: " << *SI << "\n");
    SI->eraseFromParent();
  }
}

// Main driver for dead stack slot elimination (FIXED VERSION)
bool DeadCodeElimination::eliminateDeadStackSlots(Function &F) {
  bool Changed = false;
  
  // Collect all alloca instructions
  SmallVector<AllocaInst*, 16> Allocas;
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
        Allocas.push_back(AI);
      }
    }
  }
  
  // Process each alloca
  for (AllocaInst *AI : Allocas) {
    LLVM_DEBUG(dbgs() << "Stack: Checking alloca: " << AI->getName() << "\n");
    
    // First, check if this alloca has any load instructions
    bool hasLoad = false;
    for (User *U : AI->users()) {
      if (isa<LoadInst>(U)) {
        hasLoad = true;
        LLVM_DEBUG(dbgs() << "Stack: Found load, alloca is LIVE\n");
        break;
      }
    }
    
    // If no loads, all stores to this alloca are dead
    if (!hasLoad) {
      LLVM_DEBUG(dbgs() << "Stack: Alloca " << AI->getName() 
                        << " has no loads, removing all stores\n");
      
      // Find and remove all stores to this alloca
      SmallVector<StoreInst*, 16> DeadStores;
      for (User *U : AI->users()) {
        if (StoreInst *SI = dyn_cast<StoreInst>(U)) {
          if (SI->getPointerOperand() == AI) {
            DeadStores.push_back(SI);
            LLVM_DEBUG(dbgs() << "Stack: Found dead store: " << *SI << "\n");
          }
        }
      }
      
      // Remove dead stores
      for (StoreInst *SI : DeadStores) {
        LLVM_DEBUG(dbgs() << "Stack: Removing dead store: " << *SI << "\n");
        SI->eraseFromParent();
        Changed = true;
      }
      
      // After removing stores, check if alloca has any remaining users
      if (AI->use_empty()) {
        LLVM_DEBUG(dbgs() << "Stack: Removing dead alloca: " << *AI << "\n");
        AI->eraseFromParent();
        Changed = true;
      }
    }
  }
  
  return Changed;
}




bool DeadCodeElimination::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "Running DCE on function: " << F.getName() << "\n");
  
  bool Changed = false;
  bool LocalChanged = true;
  
  // Iterate until no changes occur across all stages
  while (LocalChanged) {
    LocalChanged = false;
    
    // Stage 1: Remove trivially dead instructions
    if (eliminateDeadCode(F)) {
      LocalChanged = true;
      Changed = true;
      LLVM_DEBUG(dbgs() << "Stage 1: Made changes\n");
    }
    
    // Stage 2: Simplify control flow
    if (simplifyControlFlow(F)) {
      LocalChanged = true;
      Changed = true;
      LLVM_DEBUG(dbgs() << "Stage 2: Made changes\n");
    }
    
    // Stage 3: Eliminate dead stack slots
    if (eliminateDeadStackSlots(F)) {
      LocalChanged = true;
      Changed = true;
      LLVM_DEBUG(dbgs() << "Stage 3: Made changes\n");
    }
  }
  
  if (Changed) {
    LLVM_DEBUG(dbgs() << "DCE: Made changes to function: " << F.getName() << "\n");
  }
  
  return Changed;
}