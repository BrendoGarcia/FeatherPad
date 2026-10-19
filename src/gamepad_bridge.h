#pragma once
#include "controller_state.h"

constexpr UINT_PTR GAMEPAD_TIMER=601;
struct ControllerSource { unsigned id=0;std::wstring name,guid; };
extern bool controllerCompatibility;
extern ControllerMode defaultControllerMode;
extern std::wstring preferredControllerGuid;
void bridgeReconfigure();
void bridgeShutdown();
void bridgeTick();
void bridgeSuspendForUI();
bool bridgeOwnsUI();
bool bridgePrepareLaunch(ControllerMode requested);
std::wstring bridgeStatus();
std::vector<ControllerSource> bridgeSources();
void refreshBridgePanel();
bool isVirtualBusPath(const std::wstring& path);
int bridgeSelfTest(std::string& log);
