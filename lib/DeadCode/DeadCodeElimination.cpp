#include "DeadCodeElimination.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

#define DEBUG_TYPE "dce"

PreservedAnalyses DeadCodeElimination::run(Function &F, 
                                           FunctionAnalysisManager &AM) {
  dbgs() << "Running DeadCodeElimination on function: " << F.getName() << "\n";
  return PreservedAnalyses::all();
}

// LLVM 18 compatible pass registration
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "DeadCodeElimination", "v0.1",
    [](PassBuilder &PB) {
      // For LLVM 18, we need to use the correct callback signature
      PB.registerPipelineParsingCallback(
        [](StringRef Name, ModulePassManager &MPM,
           ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "dead-code-elimination") {
            // Create a function pass manager and add our pass
            FunctionPassManager FPM;
            FPM.addPass(DeadCodeElimination());
            // Add to module pass manager
            MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
            return true;
          }
          return false;
        });
    }};
}