#include "DeadCodeElimination.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

int main(int argc, char **argv) {
  if (argc != 3) {
    errs() << "Usage: " << argv[0] << " <input.ll> <output.ll>\n";
    return 1;
  }

  std::string InputFile = argv[1];
  std::string OutputFile = argv[2];

  LLVMContext Context;
  SMDiagnostic Err;
  
  // Load the LLVM IR file
  std::unique_ptr<Module> M = parseIRFile(InputFile, Err, Context);
  if (!M) {
    Err.print("dce_tool", errs());
    return 1;
  }

  errs() << "Loaded module: " << M->getName() << "\n";
  errs() << "Number of functions: " << M->size() << "\n";
  
  // Set up the pass manager
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  
  PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
  
  // Create the function pass manager and add our pass
  FunctionPassManager FPM;
  FPM.addPass(DeadCodeElimination());
  
  // Run the pass on each function
  ModulePassManager MPM;
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  
  errs() << "Running Dead Code Elimination...\n";
  MPM.run(*M, MAM);
  
  errs() << "Dead code elimination completed successfully.\n";

  // Write the output
  std::error_code EC;
  raw_fd_ostream Out(OutputFile, EC);
  if (EC) {
    errs() << "Error opening output file: " << EC.message() << "\n";
    return 1;
  }

  M->print(Out, nullptr);
  Out.close();

  errs() << "Output written to: " << OutputFile << "\n";
  return 0;
}
