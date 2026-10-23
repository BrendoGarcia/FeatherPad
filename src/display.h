#pragma once
#include <windows.h>
#include <string>
// All authored UI rectangles use 96-DPI logical units.
void enablePerMonitorDpi();
UINT windowDpi(HWND window);
RECT monitorWorkArea(HWND owner);
RECT logicalClient(HWND owner);
double uiScale(HWND owner);
int beginLogicalPaint(HWND owner,HDC dc);
HWND createFittedWindow(const wchar_t* className,const wchar_t* title,int width,int height,HWND owner,DWORD style=WS_CAPTION|WS_SYSMENU,DWORD exStyle=WS_EX_DLGMODALFRAME);
void registerLogicalChild(HWND owner,HWND child,RECT rectangle);
void forgetWindowLayout(HWND owner);
bool handleLayoutMessage(HWND owner,UINT message,LPARAM parameter);
extern int windowPreset;
const wchar_t* windowPresetName(int preset);
void applyWindowPreset(int preset);
void mainMinimumSize(HWND owner,MINMAXINFO* info);
