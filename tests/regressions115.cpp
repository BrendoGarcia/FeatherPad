#include "app.h"
#include <psapi.h>
namespace {
 void screenshot(HWND h,const wchar_t* path){
  RECT r{};GetClientRect(h,&r);HDC dc=GetDC(h),copy=CreateCompatibleDC(dc);
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
  void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
  if(bitmap){auto previous=SelectObject(copy,bitmap);RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);GdiFlush();BitBlt(copy,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);GdiFlush();
   BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+r.right*r.bottom*4;
   HANDLE f=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(f!=INVALID_HANDLE_VALUE){DWORD written=0;WriteFile(f,&header,sizeof(header),&written,nullptr);WriteFile(f,&info.bmiHeader,sizeof(BITMAPINFOHEADER),&written,nullptr);WriteFile(f,pixels,r.right*r.bottom*4,&written,nullptr);CloseHandle(f);}
   SelectObject(copy,previous);DeleteObject(bitmap);
  }DeleteDC(copy);ReleaseDC(h,dc);
 }
 bool disjoint(HWND a,HWND b){RECT x{},y{},overlap{};GetWindowRect(a,&x);GetWindowRect(b,&y);return !IntersectRect(&overlap,&x,&y);}
}
int regressions115(std::string& log){
 int failed=0;auto check=[&](bool good,const char* name){log+=(good?"PASS ":"FAIL ");log+=name;log+='\n';if(!good)++failed;};
 if(panel)closePanel();
 useControllerProfile(L"");controllerCompatibility=false;
 // Persist real, empty topics; migrate old category memberships without imposing defaults.
 check(saveCollection(L"",L"Hardcore"),"115 create an empty collection independently of any game");
 loadDatabase();check(collectionGameCount(L"Hardcore")==0&&std::find(collections.begin(),collections.end(),L"Hardcore")!=collections.end(),"115 empty collections survive a restart");
 check(!saveCollection(L"",L"hardcore")&&!saveCollection(L"",L"bad;topic"),"115 collection names reject duplicates and separators");
 showCollections();HDC dc=GetDC(win),canvas=CreateCompatibleDC(dc);HBITMAP bmp=CreateCompatibleBitmap(dc,1180,780);auto old=SelectObject(canvas,bmp);
 paintMain(canvas,RECT{0,0,1180,780});auto topic=std::find(collections.begin(),collections.end(),L"Hardcore");const auto index=static_cast<int>(topic-collections.begin());
 auto hit=std::find_if(hits.begin(),hits.end(),[&](const auto& item){return item.collection==index&&item.action==A_OPEN_COLLECTION;});
 check(collectionsHome&&hit!=hits.end(),"115 collections have navigable topic cards rather than a filter dialog");
 if(hit!=hits.end()){keyboardFocus=static_cast<int>(hit-hits.begin());activateMain();}
 check(!collectionsHome&&collectionFilter==L"Hardcore"&&visibleGames.empty(),"115 opening an empty topic opens its own library space");
 const auto oldLaunches=games[0].launches,oldSeconds=games[0].playedSeconds;const auto oldAvatar=avatarPath;
 openPage(Page::Editor,0);HWND memberships=GetDlgItem(panel,406);
 check(memberships&&GetWindowLongPtrW(memberships,GWL_STYLE)&LBS_EXTENDEDSEL,"115 editor exposes optional collection memberships in a native list");
 SendMessageW(memberships,LB_SETSEL,FALSE,-1);const auto member=SendMessageW(memberships,LB_FINDSTRINGEXACT,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(L"Hardcore"));
 SendMessageW(memberships,LB_SETSEL,TRUE,member);screenshot(panel,L"editor-115.bmp");action(A_SAVE_GAME);
 check(!panel&&games[0].categories==L"Hardcore"&&games[0].launches==oldLaunches&&games[0].playedSeconds==oldSeconds&&avatarPath==oldAvatar,"115 saving memberships preserves statistics and adjacent profile state");
 check(saveCollection(L"Hardcore",L"Hardcore 2")&&inCollection(games[0].categories,L"Hardcore 2"),"115 renaming a topic updates its games");
 const auto total=games.size();check(removeCollection(L"Hardcore 2")&&games.size()==total&&games[0].categories.empty(),"115 deleting a topic keeps its games and clears membership");
 action(A_ALL);openPage(Page::Editor);SetWindowTextW(GetDlgItem(panel,400),L"Unsorted optional game");SetWindowTextW(GetDlgItem(panel,401),L"C:\\NotLaunched.exe");action(A_SAVE_GAME);
 check(games.size()==total+1&&games.back().categories.empty(),"115 adding a game without a collection remains supported");
 showCollections();screenshot(win,L"collections-115.bmp");action(A_ALL);
 // Connection monitoring must be stable and must never enter the button-test screen.
 std::vector<HANDLE> fakeHandles;
 for(int i=0;i<4;++i){HANDLE h=reinterpret_cast<HANDLE>(static_cast<INT_PTR>(800+i));Device d;d.path=L"test-player-"+std::to_wstring(i);devices[h]=d;fakeHandles.push_back(h);}
 openPage(Page::Players);HWND done=GetDlgItem(panel,A_CLOSE_PANEL);SetFocus(done);
 for(int i=0;i<10;++i)refreshSettingsDevices();
 bool rows=true;for(int i=0;i<4;++i)rows=rows&&disjoint(GetDlgItem(panel,470+i),done)&&textOf(GetDlgItem(panel,470+i)).find(L"Conectado")!=std::wstring::npos;
 check(rows&&GetFocus()==done&&!GetDlgItem(panel,420),"115 four connected players do not overlap Conclude or start testing buttons");
 controllerButtons(fakeHandles[0],{7},{});check(panel&&GetFocus()==done,"115 input on players page does not steal focus or draw test text");
 screenshot(panel,L"players-115.bmp");controllerButtons(fakeHandles[0],{static_cast<USAGE>(okButton)},{});
 check(!panel,"115 controller confirm can close the connection monitor");
 openPage(Page::Players);SendMessageW(GetDlgItem(panel,A_CLOSE_PANEL),BM_CLICK,0,0);check(!panel,"115 mouse click can close connection monitor");
 openPage(Page::Players);controllerButtons(fakeHandles[0],{static_cast<USAGE>(backButton)},{});check(!panel,"115 controller back can close connection monitor");
 openPage(Page::Diagnostic);check(disjoint(GetDlgItem(panel,420),GetDlgItem(panel,A_CLOSE_PANEL)),"115 explicit input test has a bounded output area");
 controllerButtons(fakeHandles[0],{7},{});check(textOf(GetDlgItem(panel,420)).find(L"7")!=std::wstring::npos,"115 explicit test still displays input");
 SetFocus(GetDlgItem(panel,A_CLOSE_PANEL));controllerButtons(fakeHandles[0],{static_cast<USAGE>(okButton)},{});check(!panel,"115 confirm is not swallowed by input diagnostics");
 openPage(Page::Settings);check(GetDlgItem(panel,A_CONTROLS)&&GetDlgItem(panel,A_GENERAL)&&!GetDlgItem(panel,A_CAPTURE_OPEN),"115 settings home has clear sections");screenshot(panel,L"settings-115.bmp");closePanel();
 // Launch a real short-lived isolated process through ShellExecuteEx, including a modal editor/keyboard.
 Game original=games[0];wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,32768);
 games[0].path=executable;games[0].args=L"--launch-fixture";games[0].dir.clear();games[0].controllerMode=ControllerMode::Native;selection=0;
 const auto launchCount=games[0].launches;
 openPage(Page::Editor,0);openVirtualKeyboard(GetDlgItem(panel,400));launch();
 check(!panel&&!keyboardWindow&&!captureWin&&!IsWindowVisible(win)&&gameSessionActive,"115 successful game launch closes owned windows and runs in the tray");
 autoOpen=true;deviceChange(GIDC_ARRIVAL,reinterpret_cast<LPARAM>(fakeHandles[1]));
 check(!IsWindowVisible(win),"115 a controller connecting during a game cannot reopen the launcher");
 check(tray.hWnd==win&&tray.uID==1&&(tray.uFlags&NIF_ICON),"115 tray icon is registered for the active launcher window");
 Sleep(1400);libraryTick();check(games[0].launches==launchCount+1,"115 successful launches still record usage");
 const auto played=games[0].playedSeconds;original.launches=games[0].launches;original.lastLaunch=games[0].lastLaunch;original.playedSeconds=played;games[0]=original;
 show();check(IsWindowVisible(win)&&!gameSessionActive,"115 explicit tray/controller open restores the library");
 games[0].path=L"C:\\FeatherPad-Missing-Game-115.exe";selection=0;const auto countBeforeFailure=games[0].launches;launch();check(IsWindowVisible(win)&&games[0].launches==countBeforeFailure,"115 a failed game launch does not hide the launcher or count usage");games[0]=original;autoOpen=false;
 for(HANDLE h:fakeHandles)devices.erase(h);
 useControllerProfile(L"");
 // Verify actual Windows memory counters against controlled resident allocation.
 const auto before=readProcessMemory();constexpr SIZE_T allocation=16*1024*1024;
 auto* pages=static_cast<volatile BYTE*>(VirtualAlloc(nullptr,allocation,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
 if(pages)for(SIZE_T offset=0;offset<allocation;offset+=4096)pages[offset]=1;
 const auto after=readProcessMemory();
 check(before.valid&&after.valid&&after.processId==GetCurrentProcessId()&&after.privateCommitBytes>=before.privateCommitBytes+allocation-1024*1024,"115 RAM comes from actual process counters and follows a committed allocation");
 check(!after.privateResidentValid||!before.privateResidentValid||after.privateResidentBytes>=before.privateResidentBytes+8*1024*1024,"115 private resident RAM follows touched physical pages");
 if(pages)VirtualFree(const_cast<BYTE*>(pages),0,MEM_RELEASE);
 refreshSystemMetrics(true);const auto sample=currentMemory();
 check(formatMiB(1048576)==L"1,0 MiB"&&memoryHeadline().find(formatMiB(sample.privateResidentValid?sample.privateResidentBytes:sample.residentBytes))!=std::wstring::npos,"115 RAM headline uses the sampled bytes with decimal precision");
 check(wifiBars(-1)==-1&&wifiBars(0)==0&&wifiBars(1)==1&&wifiBars(25)==1&&wifiBars(26)==2&&wifiBars(75)==3&&wifiBars(76)==4&&wifiBars(100)==4,"115 Wi-Fi bars map only real driver signal quality and preserve unknown");
 NetworkSample wifi;wifi.kind=NetworkKind::WiFi;wifi.signalPercent=25;drawNetworkStatus(canvas,RECT{0,0,246,36},wifi);const auto weak=GetPixel(canvas,29,11);wifi.signalPercent=100;drawNetworkStatus(canvas,RECT{0,0,246,36},wifi);const auto strong=GetPixel(canvas,29,11);
 check(weak!=strong&&strong==RGB(100,229,193),"115 actual signal level changes the visual strength bars");
 NetworkSample cable;cable.kind=NetworkKind::Ethernet;check(networkHeadline(cable).find(L"Ethernet")!=std::wstring::npos&&cable.signalPercent==-1,"115 Ethernet uses a cable indicator without invented signal strength");
 openPage(Page::Metrics);screenshot(panel,L"metrics-115.bmp");closePanel();
 SelectObject(canvas,old);DeleteObject(bmp);DeleteDC(canvas);ReleaseDC(win,dc);
 collectionsHome=false;collectionFilter.clear();rebuildLibrary();saveDatabase();
 return failed;
}
