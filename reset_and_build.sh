#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build"

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" --config Release

gather_plugin() {
  local candidate
  for candidate in \
    "${BUILD_DIR}/DeadCodeElimination.dll" \
    "${BUILD_DIR}/DeadCodeElimination.so" \
    "${BUILD_DIR}/DeadCodeElimination.dylib" \
    "${BUILD_DIR}/Release/DeadCodeElimination.dll" \
    "${BUILD_DIR}/Release/DeadCodeElimination.so" \
    "${BUILD_DIR}/Release/DeadCodeElimination.dylib" \
    "${BUILD_DIR}/Debug/DeadCodeElimination.dll" \
    "${BUILD_DIR}/Debug/DeadCodeElimination.so" \
    "${BUILD_DIR}/Debug/DeadCodeElimination.dylib" \
    "${BUILD_DIR}/lib/DeadCode/DeadCodeElimination.so" \
    "${BUILD_DIR}/lib/DeadCode/DeadCodeElimination.dylib" \
    "${BUILD_DIR}/lib/DeadCode/DeadCodeElimination.dll" \
    "${BUILD_DIR}/lib/DeadCode/Release/DeadCodeElimination.so" \
    "${BUILD_DIR}/lib/DeadCode/Release/DeadCodeElimination.dylib" \
    "${BUILD_DIR}/lib/DeadCode/Debug/DeadCodeElimination.so" \
    "${BUILD_DIR}/lib/DeadCode/Debug/DeadCodeElimination.dylib"; do
    if [ -f "${candidate}" ]; then
      printf '%s' "${candidate}"
      return 0
    fi
  done
  return 1
}

PLUGIN_PATH="$(gather_plugin)" || {
  echo "ERROR: built plugin not found in ${BUILD_DIR}" >&2
  exit 1
}

echo "Built plugin: ${PLUGIN_PATH}"

OPT_BIN="${OPT_BIN:-opt}"
if command -v "${OPT_BIN}" >/dev/null 2>&1; then
  "${OPT_BIN}" -load-pass-plugin "${PLUGIN_PATH}" -passes=my-dce -S "${ROOT}/test.ll" -o "${BUILD_DIR}/output.ll"
  echo "Pass executed successfully. Output: ${BUILD_DIR}/output.ll"
else
  echo "WARNING: opt not found in PATH. Build succeeded but pass run was skipped."
  echo "Install LLVM opt or set OPT_BIN to the opt executable."
fi

