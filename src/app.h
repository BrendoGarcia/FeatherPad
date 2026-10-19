#pragma once
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
extern "C" {
#include <hidsdi.h>
#include <hidpi.h>
}
#include <wincodec.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include "controller_state.h"
#include "gamepad_bridge.h"

constexpr UINT TRAY=WM_APP+1, FINISH_CAPTURE=WM_APP+2;
enum Action {
 A_OPEN=200,A_EXIT,A_SETTINGS,A_ALL,A_FAVORITES,A_ADD,A_EDIT,A_PLAY,A_STAR,A_PROFILE,
 A_PREV,A_NEXT,A_CLOSE_PANEL,A_SAVE_GAME,A_DELETE_GAME,A_BROWSE_GAME,A_BROWSE_COVER,A_CLEAR_COVER,
 A_STARTUP,A_AUTOOPEN,A_CAPTURE_OPEN,A_CAPTURE_OK,A_CAPTURE_BACK,A_CAPTURE_STAR,A_DIAG,
 A_CAPTURE_SAVE,A_CAPTURE_CANCEL,A_SAVE_PROFILE,A_BROWSE_AVATAR,A_CLEAR_AVATAR,A_DRAFT_FAVORITE,
 A_COMPAT,A_COMPAT_TOGGLE,A_COMPAT_RETRY,A_COMPAT_DRIVER,A_COMPAT_HIDHIDE,A_COMPAT_MAPPINGS,A_COMPAT_DEFAULT,A_COMPAT_SOURCE
};
enum class Page { None, Editor, Settings, Profile, Diagnostic, Compatibility };
struct Game {
 std::wstring name,path,args,dir,cover,note;
 bool favorite=false;
 ControllerMode controllerMode=ControllerMode::Auto;
};
struct Device {
 std::vector<BYTE> descriptor;
 std::vector<HIDP_VALUE_CAPS> values;
 std::set<USAGE> previous;
 int direction=0;
 ULONGLONG repeat=0;
};
struct Hit { RECT r; int action; int game=-1; };
struct Art { HBITMAP bitmap=nullptr; int width=0,height=0; ULONGLONG used=0; };

extern HINSTANCE instance;
extern HWND win,panel,captureWin;
extern Page page;
extern NOTIFYICONDATAW tray;
extern std::wstring ini,dataFolder,profileName,avatarPath,notice;
extern std::vector<Game> games;
extern std::map<HANDLE,Device> devices;
extern std::vector<int> visibleGames;
extern std::vector<Hit> hits;
extern int selection,firstVisible,keyboardFocus,editorIndex,captureAction;
extern unsigned openButton,okButton,backButton,starButton,captureCandidate;
extern bool autoOpen,favoritesOnly,selfTesting;
extern std::wstring captureNotice;
extern std::set<HANDLE> captureArmed;
extern IWICImagingFactory* imaging;

// Persistence / operating system integration.
std::wstring textOf(HWND h);
std::wstring readValue(const wchar_t* section,const wchar_t* key,const wchar_t* fallback=L"");
void loadDatabase();
bool saveDatabase(HWND owner=nullptr);
bool validField(const std::wstring& value,size_t max=8000);
bool importArt(std::wstring& path,HWND owner);
bool startupEnabled();
void toggleStartup();
void launch();
void toggleFavorite();
void rebuildLibrary(int preferred=-1);
void moveSelection(int delta);
void setFilter(bool favorites);
void status(const std::wstring& message);

// Native UI.
void initTheme();
void destroyTheme();
void darkTitle(HWND h);
void show();
void hide();
void openPage(Page requested,int gameIndex=-1);
void closePanel();
void beginCapture(int action);
void closeCapture();
void commitCapture();
void action(int id);
void trayMenu();
void paintMain(HDC dc,RECT client);
void drawButton(const DRAWITEMSTRUCT* item);
LRESULT CALLBACK windowProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK panelProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK captureProc(HWND,UINT,WPARAM,LPARAM);
bool routeKey(MSG& msg);
void controllerDirection(int dx,int dy);
void activateMain();
void controllerButtons(HANDLE device,const std::set<USAGE>& current,const std::set<USAGE>& previous);

// Bounded image cache: at most six 300x420 thumbnails, released when hidden.
const Art* artFor(const std::wstring& path);
void clearArt();
void drawArt(HDC dc,const Art& art,RECT target);
bool validateArt(const std::wstring& path);
bool saveThumbnail(const std::wstring& source,const std::wstring& destination);

// Raw Input monitor.
Device* deviceFor(HANDLE device);
void rawInput(LPARAM l);
void deviceChange(WPARAM w,LPARAM l);
bool registerControllers();
int runSelfTest();
