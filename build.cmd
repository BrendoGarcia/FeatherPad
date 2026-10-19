@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
 echo Instale CMake 3.24+ ou use o GitHub Actions, sem instalar compilador no PC.
 pause
 exit /b 1
)
cmake -S . -B build -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release
if errorlevel 1 exit /b 1
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 exit /b 1
echo Pronto: build\Release\FeatherPad.exe
 echo Mantenha SDL3.dll na mesma pasta do executavel.
pause
