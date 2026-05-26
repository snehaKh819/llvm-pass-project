# Final Project Report

## Project Overview
This project implements a custom LLVM optimization pass named `DeadCodeElimination`. The pass is built as an LLVM plugin and is also compiled into a standalone tool (`dce_tool`) for local validation. It targets LLVM IR and performs code cleanup across instructions, branches, and stack slot usage.

## Objectives Completed
- Implemented dead code elimination for trivially dead instructions.
- Added same-target branch simplification.
- Added forwarding block removal with PHI predecessor updates.
- Added dead stack slot elimination for unused `alloca` values and stores.
- Added statistics tracking for removed instructions, branches, blocks, stores, and allocas.
- Added a regression test suite covering key pass behaviors.
- Added build scripts for both Unix and Windows.
- Added documentation and repeatable validation support.

## Design and Implementation
### Plugin architecture
- The pass is exposed through `llvmGetPassPluginInfo()` and registered under the pipeline name `my-dce`.
- It is built as a module named `DeadCodeElimination` and can be loaded by `opt -load-pass-plugin`.
- A standalone executable `dce_tool` is also built using the same pass implementation and LLVM libraries.

### Pass stages
1. **Stage 1: Trivial dead instruction elimination**
   - Finds and removes instructions with no side effects and no remaining uses.
   - Recursively deletes operand instructions that become dead as a result.

2. **Stage 2: Control-flow simplification**
   - Replaces conditional branches whose true and false targets are identical with an unconditional branch.
   - Removes forwarding blocks that contain only an unconditional branch to another block.
   - Updates predecessor edges and PHI node incoming values when forwarding blocks are removed.

3. **Stage 3: Dead stack slot elimination**
   - Detects `alloca` instructions that are never loaded from.
   - Removes all dead stores to those allocas.
   - Removes the now-unused `alloca` instruction.

### Pass behavior
- The pass repeatedly applies all three stages until no further change occurs.
- The main driver uses LLVM's `FunctionAnalysisManager` and returns `PreservedAnalyses::none()` when changes are made.
- The implementation uses LLVM utilities such as `isInstructionTriviallyDead`, `BranchInst`, `PHINode`, and `predecessors()`.

### Statistics
The pass exposes the following LLVM statistics when run with statistics/debug support enabled:
- `NumDeadInsts`
- `NumDeadBranches`
- `NumForwardingBlocks`
- `NumDeadStores`
- `NumDeadAllocas`

## Build and Run
### Build configuration
The project uses CMake and LLVM's CMake integration.
- Top-level `CMakeLists.txt` finds LLVM and adds `lib/DeadCode`.
- The pass links against LLVM components: `core`, `support`, `analysis`, `scalaropts`, `transformutils`, `ipo`, `passes`, and `irreader`.

### Unix-style build
```bash
chmod +x reset_and_build.sh
./reset_and_build.sh
```

### Windows build
```bat
reset_and_build.bat
```

### Running the pass
The build scripts are configured to compile the plugin and optionally run it on `test.ll`.
To invoke the plugin manually with LLVM opt:
```bash
opt -load-pass-plugin <path-to-plugin> -passes=my-dce -S test.ll -o output.ll
```

## Regression Testing
### Test harness
- `tests/run_tests.py` locates the built plugin and LLVM `opt`.
- It runs the plugin against all `.in.ll` cases in `tests/cases/`.
- Output is canonicalized and compared against corresponding `.expected.ll` files.

### Covered cases
- `dead_instruction.in.ll` — removes trivially dead instructions.
- `dead_branch.in.ll` — simplifies branches whose successors are identical.
- `forwarding_block.in.ll` — removes forwarding blocks while preserving CFG correctness.
- `unused_alloca.in.ll` — eliminates dead stack allocations and stores.
- `phi_predecessor.in.ll` — updates PHI nodes after predecessor redirection.

### Running tests
```bash
python tests/run_tests.py
```
If `opt` is not on the `PATH`, use:
```bash
python tests/run_tests.py --opt C:\path\to\opt.exe
```

## Validation
- The implementation matches the repository source code and test harness logic.
- The regression test suite provides deterministic validation across representative LLVM IR cases.
- Successful build depends on a valid LLVM installation with `cmake`, `opt`, and the LLVM CMake configuration available.

## Suggested Improvements
- Add more regression cases for complex PHI updates, switch-based control-flow folding, and load/store interactions.
- Add benchmarking to compare pass runtime and code size impact.
- Extend support to more LLVM IR constructs and stronger interprocedural dead code elimination.
- Add direct PassBuilder pipeline integration beyond plugin registration.

## Conclusion
The project delivers a working LLVM dead code elimination pass with:
- a plugin registration interface,
- a standalone validation tool,
- multi-stage code cleanup,
- statistics reporting,
- and regression testing.

This report can serve as the final project summary for evaluation submission.
