#pragma once
#include <windows.h>
constexpr UINT UPDATE_READY=WM_APP+20;
void checkForUpdates();
void handleUpdateResult();
void shutdownUpdates();
