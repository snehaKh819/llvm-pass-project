@echo off
setlocal enabledelayedexpansion

if exist build rmdir /s /q build
mkdir build

cmake -S . -B build
cmake --build build --config Release

set PLUGIN=
if exist build\DeadCodeElimination.dll set PLUGIN=build\DeadCodeElimination.dll
if exist build\Release\DeadCodeElimination.dll set PLUGIN=build\Release\DeadCodeElimination.dll
if exist build\DeadCodeElimination.so set PLUGIN=build\DeadCodeElimination.so
if exist build\Release\DeadCodeElimination.so set PLUGIN=build\Release\DeadCodeElimination.so
if "%PLUGIN%"=="" (
  echo ERROR: built plugin not found.
  exit /b 1
)

echo Built plugin: %PLUGIN%
where opt >nul 2>nul
if %ERRORLEVEL%==0 (
  opt -load-pass-plugin "%PLUGIN%" -passes=my-dce -S test.ll -o build\output.ll
  echo Pass executed successfully. Output: build\output.ll
) else (
  echo WARNING: opt not found on PATH. Build succeeded but pass run was skipped.
)
endlocal
