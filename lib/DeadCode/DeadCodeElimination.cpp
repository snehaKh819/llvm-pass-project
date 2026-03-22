#include<DeadCodeElimination.h>
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
using namespace llvm;
#define DEBUG_TYPE "dce"
PreservedAnalyses DeadCodeElimination::run(Fucntion &F,FunctionAnalysisManager &AM){
    dbgs()<<"Running Dead Code Elimination on function: "<<F.getName()<<endl;
    return PreservedAnalysis::all();
}
extern "C" ::LLVM::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo(){
    return{
        LLVM_PLUGIN_API_VERSION, "DeadCodeElimination","v0.1",[](PassBuilder &PB){
            PB.registerPipelineParsingCallback([](StringRef Name,FunctionPassManager &FPM,arrayRef<PassBuilder::PipelineElement>){
                if(Name=="dead-code-elimination"){
                    FPM.addPass(DeadCodeElimination());
                    return true;
                }
            return false;
        });
    }};
}