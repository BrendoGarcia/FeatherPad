@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
  echo Abra este arquivo pelo 'x64 Native Tools Command Prompt for VS'.
  echo Instale Visual Studio Build Tools com 'Desenvolvimento para desktop com C++'.
  pause
  exit /b 1
)
if not exist dist mkdir dist
cl /nologo /std:c++17 /O1 /MT /EHsc /W4 /DWINVER=0x0601 /D_WIN32_WINNT=0x0601 src\main.cpp /Fe:dist\FeatherPad.exe /Fo:dist\main.obj /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib shell32.lib ole32.lib comdlg32.lib advapi32.lib hid.lib
if errorlevel 1 (
  echo Falha na compilacao.
  pause
  exit /b 1
)
del dist\main.obj 2>nul
echo Pronto: dist\FeatherPad.exe
pause
