@echo off
REM ===== ビルドフォルダ作成 =====
if not exist build mkdir build
cd build

REM ===== CMake 実行 =====
cmake ..

REM ===== ビルド実行 =====
cmake --build .

REM ===== 完了したら実行 =====
echo.
echo --- Run Program ---
MyTool.exe

cd ..
