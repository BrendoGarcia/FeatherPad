#include "app.h"

HINSTANCE instance=nullptr;
HWND win=nullptr,panel=nullptr,captureWin=nullptr;
Page page=Page::None;
NOTIFYICONDATAW tray{};
std::wstring ini,dataFolder,profileName=L"Jogador",avatarPath,notice,captureNotice;
std::vector<Game> games;
std::map<HANDLE,Device> devices;
std::vector<int> visibleGames;
std::vector<Hit> hits;
std::set<HANDLE> captureArmed;
int selection=-1,firstVisible=0,keyboardFocus=-1,editorIndex=-1,captureAction=0;
unsigned openButton=13,okButton=2,backButton=3,starButton=4,captureCandidate=0;
bool autoOpen=false,favoritesOnly=false,selfTesting=false;
IWICImagingFactory* imaging=nullptr;

int WINAPI wWinMain(HINSTANCE h,HINSTANCE,LPWSTR args,int) {
 instance=h;selfTesting=std::wstring(args).find(L"--self-test")!=std::wstring::npos;
 HANDLE singleton=nullptr;
 if(!selfTesting) {
  singleton=CreateMutexW(nullptr,FALSE,L"Local\\FeatherPad.Singleton.v1");if(!singleton)return 1;
  if(GetLastError()==ERROR_ALREADY_EXISTS) {
   HWND other=FindWindowW(L"FeatherPadWindow",nullptr);if(other){ShowWindow(other,IsIconic(other)?SW_RESTORE:SW_SHOW);SetForegroundWindow(other);}CloseHandle(singleton);return 0;
  }
 }
 const HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 if(FAILED(com)){if(singleton)CloseHandle(singleton);return 1;}
 CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&imaging));
 if(!selfTesting) {
  PWSTR folder=nullptr;
  if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&folder))){if(imaging)imaging->Release();CoUninitialize();CloseHandle(singleton);return 1;}
  dataFolder=std::wstring(folder)+L"\\FeatherPad";CoTaskMemFree(folder);
  if(!CreateDirectoryW(dataFolder.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS){if(imaging)imaging->Release();CoUninitialize();CloseHandle(singleton);return 1;}
  ini=dataFolder+L"\\library.ini";loadDatabase();
  // Add a non-conflicting Favorite binding when upgrading a customized v1 setup.
  if(starButton==openButton||starButton==okButton||starButton==backButton)for(unsigned b=1;b<128;++b)if(b!=openButton&&b!=okButton&&b!=backButton){starButton=b;break;}
 }
 initTheme();
 HICON icon=LoadIconW(h,MAKEINTRESOURCEW(101));if(!icon)icon=LoadIconW(nullptr,IDI_APPLICATION);
 WNDCLASSW wc{};wc.hInstance=h;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=icon;wc.style=CS_DBLCLKS;
 wc.lpszClassName=L"FeatherPadWindow";wc.lpfnWndProc=windowProc;RegisterClassW(&wc);
 wc.lpszClassName=L"FeatherPadPanel";wc.lpfnWndProc=panelProc;RegisterClassW(&wc);
 wc.lpszClassName=L"FeatherPadCapture";wc.lpfnWndProc=captureProc;RegisterClassW(&wc);
 RECT bounds{0,0,1180,780};AdjustWindowRect(&bounds,WS_OVERLAPPEDWINDOW,FALSE);
 win=CreateWindowExW(0,L"FeatherPadWindow",L"FeatherPad 2.2 — Sua biblioteca de jogos",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nullptr,h,nullptr);
 int result=1;
 if(win) {
  darkTitle(win);rebuildLibrary();
  if(selfTesting){result=runSelfTest();if(panel)closePanel();DestroyWindow(win);}
  else {
   bridgeReconfigure();
   if(!registerControllers())MessageBoxW(win,L"Não foi possível registrar os controles. Teclado e mouse continuam disponíveis.",L"FeatherPad",MB_ICONWARNING);
   tray.cbSize=sizeof(tray);tray.hWnd=win;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=TRAY;tray.hIcon=icon;
   lstrcpyW(tray.szTip,L"FeatherPad — sua biblioteca de jogos");
   if(!Shell_NotifyIconW(NIM_ADD,&tray))status(L"Não foi possível adicionar o ícone à bandeja.");
   if(std::wstring(args).find(L"--background")==std::wstring::npos)show();
   MSG msg{};int state;
   while((state=GetMessageW(&msg,nullptr,0,0))>0){if(!routeKey(msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
   result=state==-1?1:0;
  }
 }
 shutdownUpdates();bridgeShutdown();clearArt();destroyTheme();if(imaging)imaging->Release();CoUninitialize();if(singleton)CloseHandle(singleton);
 return result;
}
