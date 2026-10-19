@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
 echo Abra no x64 Native Tools Command Prompt for VS.
 echo Instale Build Tools com Desenvolvimento para desktop com C++.
 pause
 exit /b 1
)
if not exist dist mkdir dist
rc /nologo /i src /fo dist\app.res src\app.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /O1 /MT /EHsc /W4 /utf-8 /Isrc /DWINVER=0x0601 /D_WIN32_WINNT=0x0601 src\main.cpp src\storage.cpp src\art.cpp src\ui.cpp src\input.cpp tests\smoke.cpp dist\app.res /Fe:dist\FeatherPad.exe /Fo:dist\ /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib shell32.lib ole32.lib comdlg32.lib advapi32.lib hid.lib windowscodecs.lib uuid.lib dwmapi.lib
if errorlevel 1 (
 echo Falha na compilacao.
 pause
 exit /b 1
)
del dist\*.obj dist\app.res 2>nul
echo Pronto: dist\FeatherPad.exe
pause
