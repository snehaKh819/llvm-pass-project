# Dead Code Elimination Test Suite

This directory contains regression tests for the `DeadCodeElimination` LLVM pass.

## Running tests

1. Build the plugin using `reset_and_build.sh` or your preferred CMake workflow.
2. Run:

```bash
python tests/run_tests.py
```

If LLVM `opt` is not on `PATH`, pass the binary explicitly:

```bash
python tests/run_tests.py --opt /path/to/opt
```

If the plugin is built in a nonstandard directory, use:

```bash
python tests/run_tests.py --plugin build/DeadCodeElimination.dll
```

## Regression coverage

- dead instructions
- dead same-target branches
- forwarding blocks
- unused allocas
- PHI predecessor updates
