# LLVM Dead Code Elimination Pass

A small LLVM plugin implementing a custom dead code elimination pass.

## Build

On Unix-like environments:

```bash
chmod +x reset_and_build.sh
./reset_and_build.sh
```

On Windows:

```bat
reset_and_build.bat
```

If `cmake` is not on `PATH`, install it or run from a developer shell where it is available.

## Run the pass

The build scripts attempt to run the pass on `test.ll` and emit `build/output.ll`.

## Regression tests

Use the test harness to validate the pass against regression cases:

```bash
python tests/run_tests.py
```

If `opt` is not on `PATH`, pass the location explicitly:

```bash
python tests/run_tests.py --opt C:\path\to\opt.exe
```

## What is covered

- dead instruction elimination
- same-target branch simplification
- forwarding block removal
- unused alloca/store elimination
- PHI predecessor updates for removed blocks
