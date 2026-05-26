#!/usr/bin/env python3
import argparse
import difflib
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"
CASES_DIR = ROOT / "tests" / "cases"
OUTPUT_DIR = ROOT / "tests" / "output"
PLUGIN_EXTENSIONS = ["dll", "so", "dylib"]


def find_plugin(plugin_path: str | None = None) -> Path:
    if plugin_path:
        plugin = Path(plugin_path)
        if plugin.exists():
            return plugin
        raise FileNotFoundError(f"Plugin not found at {plugin}")

    candidates = [BUILD_DIR, BUILD_DIR / "Release", BUILD_DIR / "Debug"]
    for directory in candidates:
        for ext in PLUGIN_EXTENSIONS:
            candidate = directory / f"DeadCodeElimination.{ext}"
            if candidate.exists():
                return candidate

    raise FileNotFoundError(
        "Built plugin not found. Run the build script first or pass --plugin.")


def find_opt(opt_path: str | None = None) -> Path:
    if opt_path:
        opt = Path(opt_path)
        if opt.exists():
            return opt
        raise FileNotFoundError(f"opt not found at {opt}")

    for name in ["opt", "opt.exe"]:
        candidate = shutil.which(name)
        if candidate:
            return Path(candidate)

    raise FileNotFoundError("opt not found on PATH. Install LLVM and ensure opt is available.")


def canonicalize(ir_text: str) -> str:
    normalized = []
    for line in ir_text.splitlines():
        line = line.strip()
        if not line:
            continue
        if line.startswith(";"):
            continue
        if line.startswith("target datalayout"):
            continue
        if line.startswith("target triple"):
            continue
        if line.startswith("attributes"):
            continue
        if line.startswith("!llvm"):
            continue
        normalized.append(line)
    return "\n".join(normalized)


def run_one_case(opt_bin: Path, plugin: Path, inp: Path, expected: Path, actual: Path) -> bool:
    actual.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([
        str(opt_bin),
        "-load-pass-plugin",
        str(plugin),
        "-passes=my-dce",
        "-S",
        str(inp),
        "-o",
        str(actual),
    ], check=True)

    actual_text = canonicalize(actual.read_text(encoding="utf-8"))
    expected_text = canonicalize(expected.read_text(encoding="utf-8"))
    if actual_text == expected_text:
        return True

    print(f"FAIL: {inp.name}")
    diff = difflib.unified_diff(
        expected_text.splitlines(),
        actual_text.splitlines(),
        fromfile="expected",
        tofile="actual",
        lineterm="",
    )
    print("\n".join(diff))
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Run DeadCodeElimination regression tests")
    parser.add_argument("--plugin", help="Built DeadCodeElimination plugin path")
    parser.add_argument("--opt", help="Path to opt executable")
    args = parser.parse_args()

    try:
        plugin = find_plugin(args.plugin)
        opt_bin = find_opt(args.opt)
    except FileNotFoundError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    print(f"Using plugin: {plugin}")
    print(f"Using opt: {opt_bin}")

    failed = 0
    for input_file in sorted(CASES_DIR.glob("*.in.ll")):
        expected_file = input_file.with_suffix(".expected.ll")
        actual_file = OUTPUT_DIR / input_file.name.replace(".in.ll", ".actual.ll")
        if not expected_file.exists():
            print(f"Missing expected file for {input_file.name}", file=sys.stderr)
            failed += 1
            continue

        if not run_one_case(opt_bin, plugin, input_file, expected_file, actual_file):
            failed += 1

    print("\nTest summary:")
    print(f"  passed: {len(list(CASES_DIR.glob('*.in.ll'))) - failed}")
    print(f"  failed: {failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
