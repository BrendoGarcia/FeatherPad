#pragma once
#include <windows.h>
#include <string>
extern HWND keyboardWindow;
extern std::wstring searchQuery;
extern bool fullscreen;
bool gameMatchesSearch(const std::wstring& name,const std::wstring& query);
void openVirtualKeyboard(HWND editTarget=nullptr);
void closeVirtualKeyboard(bool apply);
void keyboardDirection(int dx,int dy);
void keyboardConfirm();
bool keyboardRoute(MSG& msg);
void toggleFullscreen();
void leaveFullscreen();
POINT canvasPoint(POINT p);
void paintCanvas(HDC dc,RECT physical);
