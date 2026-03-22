#ifndef DEADCODEELIMINATION_H
#define DEADCODEELIMINATION.H
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
namespace llvm{
    class DeadCodeElimination:public PassInfoMixin<DeadCodeElimination>{
        public:
            PreservedAnalyses run(Function &f,FunctionAnalysisManager &AM);
    };
}
#endif