#include "DeadCodeElimination.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

#define DEBUG_TYPE "dce"

STATISTIC(NumDeadInsts, "Number of dead instructions removed");
STATISTIC(NumDeadBranches, "Number of same-target branches simplified");
STATISTIC(NumForwardingBlocks, "Number of forwarding blocks removed");
STATISTIC(NumDeadStores, "Number of dead stores removed");
STATISTIC(NumDeadAllocas, "Number of dead allocas removed");

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "DeadCodeElimination", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "my-dce") {
                    FPM.addPass(DeadCodeElimination());
                    return true;
                  }
                  return false;
                });
          }};
}



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
    ++NumDeadInsts;
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
      ++NumDeadBranches;
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
      ++NumForwardingBlocks;
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



bool DeadCodeElimination::isAllocaDead(AllocaInst *AI) {
  
  bool hasLoad = false;
  
  for (User *U : AI->users()) {
    
    if (LoadInst *LI = dyn_cast<LoadInst>(U)) {
      hasLoad = true;
      LLVM_DEBUG(dbgs() << "Stack: Alloca " << AI->getName() 
                        << " has load: " << *LI << "\n");
      break;  
    }
  }
  
  
  return !hasLoad;
}


void DeadCodeElimination::removeDeadStores(AllocaInst *AI) {
  
  SmallVector<StoreInst*, 16> DeadStores;
  
  for (User *U : AI->users()) {
    if (StoreInst *SI = dyn_cast<StoreInst>(U)) {
      
      if (SI->getPointerOperand() == AI) {
        DeadStores.push_back(SI);
        LLVM_DEBUG(dbgs() << "Stack: Found dead store: " << *SI << "\n");
      }
    }
  }
  
  
  for (StoreInst *SI : DeadStores) {
    LLVM_DEBUG(dbgs() << "Stack: Removing dead store: " << *SI << "\n");
    SI->eraseFromParent();
  }
}


bool DeadCodeElimination::eliminateDeadStackSlots(Function &F) {
  bool Changed = false;
  
  SmallVector<AllocaInst*, 16> Allocas;
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
        Allocas.push_back(AI);
      }
    }
  }
  
  
  for (AllocaInst *AI : Allocas) {
    LLVM_DEBUG(dbgs() << "Stack: Checking alloca: " << AI->getName() << "\n");
    
    
    bool hasLoad = false;
    for (User *U : AI->users()) {
      if (isa<LoadInst>(U)) {
        hasLoad = true;
        LLVM_DEBUG(dbgs() << "Stack: Found load, alloca is LIVE\n");
        break;
      }
    }
    
    
    if (!hasLoad) {
      LLVM_DEBUG(dbgs() << "Stack: Alloca " << AI->getName() 
                        << " has no loads, removing all stores\n");
      
      
      SmallVector<StoreInst*, 16> DeadStores;
      for (User *U : AI->users()) {
        if (StoreInst *SI = dyn_cast<StoreInst>(U)) {
          if (SI->getPointerOperand() == AI) {
            DeadStores.push_back(SI);
            LLVM_DEBUG(dbgs() << "Stack: Found dead store: " << *SI << "\n");
          }
        }
      }
      
      
      for (StoreInst *SI : DeadStores) {
        LLVM_DEBUG(dbgs() << "Stack: Removing dead store: " << *SI << "\n");
        SI->eraseFromParent();
        ++NumDeadStores;
        Changed = true;
      }
      
      
      if (AI->use_empty()) {
        LLVM_DEBUG(dbgs() << "Stack: Removing dead alloca: " << *AI << "\n");
        AI->eraseFromParent();
        ++NumDeadAllocas;
        Changed = true;
      }
    }
  }
  
  return Changed;
}




PreservedAnalyses DeadCodeElimination::run(Function &F, FunctionAnalysisManager &AM) {
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
  
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}