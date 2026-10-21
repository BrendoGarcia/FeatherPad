#include "app.h"

namespace {
 bool writeBytes(const std::wstring& path,const void* data,DWORD size) {
  HANDLE f=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(f==INVALID_HANDLE_VALUE)return false;
  DWORD n=0;bool ok=WriteFile(f,data,size,&n,nullptr)&&n==size;CloseHandle(f);return ok;
 }
 void cleanup(const std::wstring& folder) {
  WIN32_FIND_DATAW f{};HANDLE search=FindFirstFileW((folder+L"\\*").c_str(),&f);
  if(search!=INVALID_HANDLE_VALUE){do{if(lstrcmpW(f.cFileName,L".")==0||lstrcmpW(f.cFileName,L"..")==0)continue;auto path=folder+L"\\"+f.cFileName;if(f.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)cleanup(path);else DeleteFileW(path.c_str());}while(FindNextFileW(search,&f));FindClose(search);}
  RemoveDirectoryW(folder.c_str());
 }
}
int runSelfTest() {
 std::string log;int failed=0;
 auto check=[&](bool ok,const char* name){log+=(ok?"PASS ":"FAIL ");log+=name;log+='\n';if(!ok)++failed;};
 wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);GUID id{};CoCreateGuid(&id);wchar_t guid[40]{};StringFromGUID2(id,guid,40);
 dataFolder=std::wstring(temp)+L"FeatherPad-Test-"+guid;CreateDirectoryW(dataFolder.c_str(),nullptr);ini=dataFolder+L"\\library.ini";
 const std::wstring legacy=L"\ufeff[Settings]\r\nOpen=13\r\nConfirm=2\r\nBack=3\r\n[Games]\r\nCount=1\r\n[Game0]\r\nName=Legacy Game\r\nPath=C:\\Games\\Test.exe\r\nArgs=\"--name \\\"old\\\"\"\r\nDir=C:\\Games\r\n";
 check(writeBytes(ini,legacy.data(),static_cast<DWORD>(legacy.size()*sizeof(wchar_t))),"write isolated v1 fixture");
 loadDatabase();check(games.size()==1&&games[0].name==L"Legacy Game"&&games[0].path==L"C:\\Games\\Test.exe","load v1 library without loss");
 check(!controllerCompatibility&&defaultControllerMode==ControllerMode::Xbox&&(games.empty()||games[0].controllerMode==ControllerMode::Auto),"legacy libraries do not enable emulation or lose games during upgrade");
 if(games.empty()){cleanup(dataFolder);return 1;}
 check(profileName==L"Jogador"&&!games[0].favorite&&games[0].cover.empty(),"new fields default safely during upgrade");
 games[0].args=L"--name \"a b\"";games[0].note=L"Ação, aventura e exploração";games[0].favorite=true;profileName=L"Brendo \"Player\"";
 check(saveDatabase(),"atomic v2 database write");loadDatabase();
 check(games.size()==1&&games[0].favorite&&games[0].args==L"--name \"a b\""&&profileName==L"Brendo \"Player\"","Unicode, quotes and favorites round trip");
 games[0].controllerMode=ControllerMode::PlayStation;defaultControllerMode=ControllerMode::PlayStation;controllerCompatibility=true;preferredControllerGuid=L"test-controller-guid";
 check(saveDatabase(),"write controller compatibility preferences");loadDatabase();
 check(controllerCompatibility&&defaultControllerMode==ControllerMode::PlayStation&&games[0].controllerMode==ControllerMode::PlayStation&&preferredControllerGuid==L"test-controller-guid","global and per-game controller choices survive restart");
 controllerCompatibility=false;defaultControllerMode=ControllerMode::Xbox;preferredControllerGuid.clear();
 check(GetFileAttributesW((ini+L".backup").c_str())!=INVALID_FILE_ATTRIBUTES,"pre-upgrade database backup exists");
 Game second;second.name=L"Second Game";second.path=L"C:\\Games\\Second.exe";games.push_back(second);
 setFilter(true);check(visibleGames.size()==1&&selection==0,"favorites filter preserves game identity");
 setFilter(false);moveSelection(-1);check(selection==1,"carousel wraps backward");moveSelection(1);check(selection==0,"carousel wraps forward");
 toggleFavorite();setFilter(true);check(visibleGames.empty()&&selection==-1,"removing final favorite leaves valid empty view");setFilter(false);
 check(!validField(L"broken\nvalue")&&validField(L"--path \"a b\""),"reject line injection without rejecting quoted arguments");

 games[0].favorite=true;
 check(gameMatchesSearch(L"The Witcher 3",L"witcher")&&gameMatchesSearch(L"Ação",L"acao")&&!gameMatchesSearch(L"Portal",L"witcher"),"search ignores case and accents");
 searchQuery=L"legacy";rebuildLibrary();check(visibleGames.size()==1,"search filters by title");
 setFilter(true);check(visibleGames.size()==1,"search composes with favorites");searchQuery=L"no matching title";rebuildLibrary();check(visibleGames.empty()&&selection==-1,"empty search clears selection safely");
 searchQuery.clear();setFilter(false);
 openVirtualKeyboard();check(keyboardWindow!=nullptr,"controller keyboard opens for search");
 SetWindowTextW(GetDlgItem(keyboardWindow,690),L"Second");closeVirtualKeyboard(false);check(searchQuery.empty(),"cancel search preserves previous query");
 openVirtualKeyboard();SetWindowTextW(GetDlgItem(keyboardWindow,690),L"Second");closeVirtualKeyboard(true);check(searchQuery==L"Second"&&visibleGames.size()==1,"virtual keyboard applies search");
 searchQuery.clear();setFilter(false);
 HWND textTarget=CreateWindowExW(0,L"EDIT",L"old",WS_CHILD,0,0,200,30,win,nullptr,instance,nullptr);
 SendMessageW(textTarget,EM_SETLIMITTEXT,40,0);openVirtualKeyboard(textTarget);SetWindowTextW(GetDlgItem(keyboardWindow,690),L"new text");closeVirtualKeyboard(true);check(textOf(textTarget)==L"new text","virtual keyboard edits native fields");DestroyWindow(textTarget);
 ShowWindow(win,SW_SHOWNORMAL);RECT beforeFull{};GetWindowRect(win,&beforeFull);toggleFullscreen();
 check(fullscreen&&!(GetWindowLongPtrW(win,GWL_STYLE)&WS_CAPTION),"fullscreen removes window borders");leaveFullscreen();RECT afterFull{};GetWindowRect(win,&afterFull);check(!fullscreen&&EqualRect(&beforeFull,&afterFull),"fullscreen restores window bounds");
 // A real BMP fixture exercises the Windows image decoder and GDI cache.
 BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+16;
 BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=2;info.biHeight=2;info.biPlanes=1;info.biBitCount=32;info.biCompression=BI_RGB;
 std::vector<BYTE> bmp(header.bfSize);CopyMemory(bmp.data(),&header,sizeof(header));CopyMemory(bmp.data()+sizeof(header),&info,sizeof(info));
 for(size_t i=header.bfOffBits;i<bmp.size();i+=4){bmp[i]=90;bmp[i+1]=200;bmp[i+2]=50;}
 auto image=dataFolder+L"\\test.bmp";check(writeBytes(image,bmp.data(),static_cast<DWORD>(bmp.size()))&&validateArt(image),"decode a real BMP with WIC");
 auto imported=image;check(importArt(imported,win)&&imported!=image,"copy artwork into managed profile folder");
 const DWORD before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
 for(int i=0;i<12;++i){auto copy=dataFolder+L"\\cover"+std::to_wstring(i)+L".bmp";CopyFileW(image.c_str(),copy.c_str(),FALSE);artFor(copy);}
 const DWORD cached=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
 check(cached<=before+6,"thumbnail cache has a six-bitmap bound");clearArt();
 check(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=before,"clearing artwork releases GDI bitmaps");
 games[0].cover=imported;games[0].favorite=true;games[0].name=L"Minha aventura";rebuildLibrary(0);
 HDC screen=GetDC(win),memory=CreateCompatibleDC(screen);HBITMAP bitmap=CreateCompatibleBitmap(screen,1180,780);auto old=SelectObject(memory,bitmap);
 paintMain(memory,RECT{0,0,1180,780});check(!hits.empty(),"paint native library with cover, profile and favorite");
 keyboardFocus=-1;controllerDirection(0,-1);check(keyboardFocus>=0,"controller can leave carousel and reach interface actions");
 selection=0;keyboardFocus=-1;controllerDirection(1,0);check(selection==1,"horizontal controller movement advances library carousel");
 selection=0;keyboardFocus=-1;
 // Render the actual mint primary button into a bitmap. Its focused edge must
 // be white and different from its unfocused edge, rather than mint-on-mint.
 HWND sample=CreateWindowExW(0,L"BUTTON",L"Salvar",WS_CHILD|BS_OWNERDRAW,0,0,180,48,win,nullptr,instance,nullptr);
 DRAWITEMSTRUCT item{};item.CtlType=ODT_BUTTON;item.CtlID=A_SAVE_GAME;item.hwndItem=sample;item.hDC=memory;item.rcItem=RECT{0,0,180,48};
 drawButton(&item);const COLORREF unfocused=GetPixel(memory,4,24);
 item.itemState=ODS_FOCUS;drawButton(&item);
 check(GetPixel(memory,4,24)==RGB(255,255,255)&&GetPixel(memory,4,24)!=unfocused,"primary-button focus has a contrasting white edge");
 item.CtlID=A_AUTOOPEN;item.itemState=0;drawButton(&item);const COLORREF toggleEdge=GetPixel(memory,4,24);
 item.itemState=ODS_FOCUS;drawButton(&item);
 check(GetPixel(memory,4,24)==RGB(255,255,255)&&GetPixel(memory,4,24)!=toggleEdge,"settings-toggle focus uses the same high-contrast edge");
 DestroyWindow(sample);
 SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(win,screen);
 // Opening settings must preserve the main window's state and normal bounds.
 ShowWindow(win,SW_SHOWNORMAL);RECT normalBounds{};GetWindowRect(win,&normalBounds);
 openPage(Page::Settings);closePanel();RECT afterSettings{};GetWindowRect(win,&afterSettings);
 check(!IsZoomed(win)&&EqualRect(&normalBounds,&afterSettings),"settings preserve normal window geometry");
 ShowWindow(win,SW_MAXIMIZE);openPage(Page::Settings);
 check(IsZoomed(win)!=FALSE,"opening settings preserves maximization");closePanel();
 check(IsZoomed(win)!=FALSE,"closing settings preserves maximization");
 hide();show();check(IsZoomed(win)!=FALSE,"hiding and showing preserves maximization");
 ShowWindow(win,SW_MINIMIZE);show();
 check(IsWindowVisible(win)&&!IsIconic(win)&&IsZoomed(win),"restoring a minimized maximized window preserves maximization");
 ShowWindow(win,SW_SHOWNORMAL);
 openPage(Page::Settings);check(panel&&IsWindow(panel),"open dedicated settings window");
 beginCapture(A_CAPTURE_OPEN);check(captureWin&&captureAction==A_CAPTURE_OPEN,"open action-specific controller popup");
 HANDLE fake=reinterpret_cast<HANDLE>(static_cast<INT_PTR>(123));
 controllerButtons(fake,{6},{});check(captureCandidate==0,"held input cannot bind before a release report");
 controllerButtons(fake,{}, {6});controllerButtons(fake,{static_cast<USAGE>(okButton)},{});
 check(captureCandidate==0,"reject a button already assigned to another action");
 controllerButtons(fake,{},{});controllerButtons(fake,{6},{});
 check(captureCandidate==6&&openButton==13,"capture waits for explicit save before changing binding");
 commitCapture();check(!captureWin&&openButton==6,"save captured button and close only capture popup");
 beginCapture(A_CAPTURE_OPEN);controllerButtons(fake,{},{});controllerButtons(fake,{7},{});closeCapture();
 check(openButton==6,"cancel capture preserves saved binding");closePanel();loadDatabase();check(openButton==6,"saved binding survives reload");
 openPage(Page::Profile);check(panel&&page==Page::Profile,"profile editor opens independently");closePanel();
 openPage(Page::Editor,0);check(GetDlgItem(panel,400)!=nullptr&&GetDlgItem(panel,A_SAVE_GAME)!=nullptr,"game editor fields and save control exist");
 SetWindowTextW(GetDlgItem(panel,400),L"Edited through native UI");action(A_SAVE_GAME);
 check(!panel&&games[0].name==L"Edited through native UI","native game editor saves and closes");
 openPage(Page::Diagnostic);controllerButtons(fake,{2,3},{});
 check(textOf(GetDlgItem(panel,420)).find(L"2")!=std::wstring::npos,"diagnostic shows inputs without rebinding");closePanel();
 openPage(Page::Compatibility);check(GetDlgItem(panel,A_COMPAT_TOGGLE)!=nullptr&&GetDlgItem(panel,A_COMPAT_DEFAULT)!=nullptr,"controller compatibility panel exposes activation and default format");closePanel();
 failed+=controllerStateSelfTest(log);failed+=bridgeSelfTest(log);
 clearArt();cleanup(dataFolder);log+="\nFailures: "+std::to_string(failed)+"\n";
 writeBytes(L"self-test.log",log.data(),static_cast<DWORD>(log.size()));return failed?1:0;
}
