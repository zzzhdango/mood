@echo off
setlocal enabledelayedexpansion
title OBYS MOOD - Windows Build and Installer Generator

echo ============================================================
echo   OBYS MOOD v1.0.0 - Automated Build and Setup.exe Builder
echo ============================================================
echo.

cd /d "%~dp0.."

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] CMake is not installed or not in PATH.
    echo Please install CMake 3.22+ from https://cmake.org/download/
    pause
    exit /b 1
)

echo [1/3] Configuring CMake and fetching JUCE 8.0.4...
cmake -B build -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed. Make sure Visual Studio C++ Build Tools are installed.
    pause
    exit /b 1
)

echo [2/3] Compiling OBYS MOOD (VST3 + Standalone)...
cmake --build build --config Release --parallel
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed.
    pause
    exit /b 1
)

echo [3/3] Packaging Windows Installer (.exe)...
set ISCC_PATH=""
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set ISCC_PATH="%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set ISCC_PATH="%ProgramFiles%\Inno Setup 6\ISCC.exe"

if not %ISCC_PATH%=="" (
    %ISCC_PATH% "Installer\OBYS_MOOD.iss"
    echo.
    echo [SUCCESS] Installer created at: dist\OBYS_MOOD_Setup_v1.0.0.exe
) else (
    echo [WARN] Inno Setup 6 not found. Installing VST3 directly to Common Files...
    if not exist "%CommonProgramFiles%\VST3" mkdir "%CommonProgramFiles%\VST3"
    xcopy /E /I /Y "build\OBYS_MOOD_artefacts\Release\VST3\OBYS MOOD.vst3" "%CommonProgramFiles%\VST3\OBYS MOOD.vst3"
    echo [DONE] VST3 installed directly into %CommonProgramFiles%\VST3\
)

pause
