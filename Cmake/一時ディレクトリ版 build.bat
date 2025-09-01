@echo off
setlocal

REM ===== 一時ビルドフォルダを TEMP に作成 =====
set BUILD_DIR=%TEMP%\MyToolBuild

REM 古いものが残っていたら削除
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

REM ===== ソースディレクトリに移動 (バッチファイルのある場所) =====
set SRC_DIR=%~dp0

REM ===== CMake 実行 (Release設定) =====
cd /d "%BUILD_DIR%"
cmake -DCMAKE_BUILD_TYPE=Release "%SRC_DIR%"

REM ===== ビルド実行 (Release) =====
cmake --build . --config Release

REM ===== exe をソースフォルダにコピー =====
if exist "%BUILD_DIR%\Release\MyTool.exe" (
    copy /Y "%BUILD_DIR%\Release\MyTool.exe" "%SRC_DIR%MyTool.exe"
) else (
    copy /Y "%BUILD_DIR%\MyTool.exe" "%SRC_DIR%MyTool.exe"
)

REM ===== TEMP のビルドフォルダを削除 =====
cd /d "%SRC_DIR%"
rmdir /s /q "%BUILD_DIR%"

REM ===== 完成物を実行 =====
echo.
echo --- Run Program ---
MyTool.exe

endlocal
