#include "app.h"
#include <cmath>
namespace {
 bool contained(RECT inner,RECT outer){return inner.left>=outer.left&&inner.top>=outer.top&&inner.right<=outer.right+1&&inner.bottom<=outer.bottom+1;}
 bool controlsFit(HWND owner){RECT outer{};GetClientRect(owner,&outer);struct State{HWND owner;RECT outer;bool good=true;} state{owner,outer,true};
  EnumChildWindows(owner,[](HWND child,LPARAM data)->BOOL{auto& state=*reinterpret_cast<State*>(data);RECT r{};GetWindowRect(child,&r);MapWindowPoints(nullptr,state.owner,reinterpret_cast<POINT*>(&r),2);state.good=state.good&&contained(r,state.outer);return TRUE;},reinterpret_cast<LPARAM>(&state));return state.good;
 }
 void snapshot(HWND h,const wchar_t* path){
  RECT r{};GetClientRect(h,&r);HDC dc=GetDC(h),copy=CreateCompatibleDC(dc);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
  void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);if(bitmap){auto old=SelectObject(copy,bitmap);RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);GdiFlush();BitBlt(copy,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);GdiFlush();BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+r.right*r.bottom*4;HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);if(file!=INVALID_HANDLE_VALUE){DWORD n;WriteFile(file,&header,sizeof(header),&n,nullptr);WriteFile(file,&info.bmiHeader,sizeof(BITMAPINFOHEADER),&n,nullptr);WriteFile(file,pixels,r.right*r.bottom*4,&n,nullptr);CloseHandle(file);}SelectObject(copy,old);DeleteObject(bitmap);}DeleteDC(copy);ReleaseDC(h,dc);
 }
 bool makeCropFixture(const std::wstring& path){const int width=900,height=420;std::vector<DWORD> pixels(width*height);for(int y=0;y<height;++y)for(int x=0;x<width;++x)pixels[y*width+x]=x<300?0x00ff0000:x<600?0x0000ff00:0x000000ff;
  BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=width;info.biHeight=-height;info.biPlanes=1;info.biBitCount=32;BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(info);header.bfSize=header.bfOffBits+static_cast<DWORD>(pixels.size()*4);HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);if(file==INVALID_HANDLE_VALUE)return false;DWORD n;bool good=WriteFile(file,&header,sizeof(header),&n,nullptr)&&WriteFile(file,&info,sizeof(info),&n,nullptr)&&WriteFile(file,pixels.data(),static_cast<DWORD>(pixels.size()*4),&n,nullptr);CloseHandle(file);return good;
 }
 COLORREF center(const std::wstring& path){const Art* art=artFor(path);if(!art)return CLR_INVALID;HDC dc=CreateCompatibleDC(nullptr);auto old=SelectObject(dc,art->bitmap);const auto color=GetPixel(dc,art->width/2,art->height/2);SelectObject(dc,old);DeleteDC(dc);return color;}
}
int displayFeaturesTest(std::string& log){
 int failed=0;auto check=[&](bool good,const char* name){log+=(good?"PASS ":"FAIL ");log+=name;log+='\n';if(!good)++failed;};
 if(panel)closePanel();
 if(fullscreen)leaveFullscreen();
 const auto oldGames=games;const auto oldView=libraryView;const int oldPreset=windowPreset;
 games.clear();for(int i=0;i<120;++i){Game game;game.name=L"Game "+std::to_wstring(i);game.path=L"C:\\fixture.exe";games.push_back(game);}setFilter(false);selection=0;
 const RECT work=monitorWorkArea(win);ShowWindow(win,SW_RESTORE);SetWindowPos(win,nullptr,work.left,work.top,960,std::min<LONG>(600,work.bottom-work.top),SWP_NOZORDER);show();
 HDC screen=GetDC(win),dc=CreateCompatibleDC(screen);HBITMAP bitmap=CreateCompatibleBitmap(screen,1920,1080);auto previous=SelectObject(dc,bitmap);
 auto paint=[&]{RECT physical{};GetClientRect(win,&physical);paintCanvas(dc,physical);};
 for(const auto view:{LibraryView::Carousel,LibraryView::Grid,LibraryView::List}){setLibraryView(view);paint();bool fit=true;RECT physical{};GetClientRect(win,&physical);POINT edge=canvasPoint(POINT{physical.right,physical.bottom});RECT logical{0,0,edge.x,edge.y};for(const auto& hit:hits)fit=fit&&contained(hit.r,logical);check(fit,"all library actions and game hit regions fit compact 960x600 window");}
 setLibraryView(LibraryView::Grid);selection=0;paint();const int columns=libraryColumns;controllerDirection(0,1);check(selection==columns,"grid down moves to same column on next row");
 for(int i=0;i<35;++i){controllerDirection(0,1);paint();}check(selection>80&&std::any_of(hits.begin(),hits.end(),[](const auto& hit){return hit.game==selection;}),"grid scrolls beyond initial page and keeps selected game visible");
 snapshot(win,L"grid-120.bmp");setLibraryView(LibraryView::List);selection=0;paint();for(int i=0;i<40;++i){controllerDirection(0,1);paint();}check(selection==40&&firstVisible>0&&std::any_of(hits.begin(),hits.end(),[](const auto& hit){return hit.game==selection;}),"list controller navigation scrolls large library");snapshot(win,L"list-120.bmp");
 searchQuery=L"Game 119";rebuildLibrary();check(visibleGames.size()==1&&selection==119,"search remains valid in list view");setLibraryView(LibraryView::Grid);check(visibleGames.size()==1&&selection==119,"changing view preserves query and selected game");searchQuery.clear();rebuildLibrary();
 windowPreset=1;saveDatabase();loadDatabase();check(libraryView==LibraryView::Grid&&windowPreset==1&&games.size()==120,"view and display preference survive persistence without losing games");
 for(int preset=0;preset<6;++preset){applyWindowPreset(preset);RECT bounds{};GetWindowRect(win,&bounds); // Maximized DWM borders may extend beyond work area.
  check(preset==5?IsZoomed(win)!=FALSE:contained(bounds,work),"standard window preset respects monitor work area and taskbar");}
 applyWindowPreset(1);
 for(Page p:{Page::Settings,Page::Editor,Page::Controls,Page::Players,Page::General,Page::Display,Page::Compatibility,Page::Metrics,Page::Statistics}){openPage(p,p==Page::Editor||p==Page::Statistics?0:-1);RECT bounds{};GetWindowRect(panel,&bounds);check(panel&&contained(bounds,monitorWorkArea(panel))&&controlsFit(panel),"settings/editor controls and Conclude fit current monitor");closePanel();}
 // Exercise 100%, 150%, 200% transforms and native font sizing on a bounded test window.
 HWND scaled=createFittedWindow(L"FeatherPadPanel",L"DPI fixture",200,160,win);HWND child=CreateWindowExW(0,L"STATIC",L"Scale",WS_CHILD|WS_VISIBLE,0,0,0,0,scaled,nullptr,instance,nullptr);registerLogicalChild(scaled,child,RECT{20,20,180,60});
 for(int dpi:{96,144,192}){const int width=200*dpi/96,height=160*dpi/96;RECT frame{0,0,200,160};RECT outer{};GetWindowRect(scaled,&outer);RECT current{};GetClientRect(scaled,&current);frame.right=width+(outer.right-outer.left-current.right);frame.bottom=height+(outer.bottom-outer.top-current.bottom);SetWindowPos(scaled,nullptr,work.left,work.top,frame.right,frame.bottom,SWP_NOZORDER|SWP_NOACTIVATE);
  HDC transformed=GetDC(scaled);int saved=beginLogicalPaint(scaled,transformed);POINT endpoint{200,160};LPtoDP(transformed,&endpoint,1);RestoreDC(transformed,saved);ReleaseDC(scaled,transformed);LOGFONTW font{};GetObjectW(reinterpret_cast<HFONT>(SendMessageW(child,WM_GETFONT,0,0)),sizeof(font),&font);
  check(std::abs(endpoint.x-width)<=1&&std::abs(endpoint.y-height)<=1&&std::abs(-font.lfHeight-static_cast<int>(std::lround(15*dpi/96.0)))<=1&&controlsFit(scaled),"100/150/200 percent transforms match native font and control geometry");
 }
 DestroyWindow(scaled);
 openPage(Page::Display);HWND focused=GetDlgItem(panel,A_CLOSE_PANEL);SetFocus(focused);RECT changed{};GetWindowRect(panel,&changed);changed.right=changed.left+720;changed.bottom=changed.top+650;SendMessageW(panel,WM_DPICHANGED,MAKEWPARAM(144,144),reinterpret_cast<LPARAM>(&changed));check(controlsFit(panel)&&GetFocus()==focused,"DPI-change suggested bounds relayout native children without losing focus");snapshot(panel,L"display-120.bmp");closePanel();
 openPage(Page::Editor,0);HWND name=GetDlgItem(panel,400);const auto before=textOf(name);openVirtualKeyboard(name);check(controlsFit(keyboardWindow),"virtual keyboard scales all keys and actions into monitor work area");closeVirtualKeyboard(false);
 const auto source=dataFolder+L"\\crop-source.bmp",left=dataFolder+L"\\crop-left.png",right=dataFolder+L"\\crop-right.png";
 check(makeCropFixture(source),"create source-resolution cover crop fixture");CoverCrop crop;crop.x=0;check(saveCroppedThumbnail(source,left,crop)&&center(left)==RGB(255,0,0),"cover import crops left source region before reducing to 300x420");crop.x=1;check(saveCroppedThumbnail(source,right,crop)&&center(right)==RGB(0,0,255),"cover framing changes imported pixels instead of only moving preview");const auto* art=artFor(right);check(art&&art->width==300&&art->height==420,"cropped covers keep bounded 300x420 dimensions");
 CoverCrop extreme;extreme.x=-10;extreme.y=20;extreme.zoom=100;const auto region=coverCropRect(900,420,extreme);check(contained(region,RECT{0,0,900,420})&&region.right>region.left&&region.bottom>region.top,"crop clamps position and zoom to valid source bounds");
 openCoverPreview(source);check(coverPreviewWindow&&controlsFit(coverPreviewWindow)&&!IsWindowEnabled(panel),"cover preview is modal and all adjustment actions fit");snapshot(coverPreviewWindow,L"cover-120.bmp");controllerButtons(nullptr,{static_cast<USAGE>(backButton)},{});check(!coverPreviewWindow&&IsWindowEnabled(panel)&&textOf(name)==before,"controller back cancels crop without changing editor");
 openCoverPreview(source);controllerButtons(nullptr,{static_cast<USAGE>(okButton)},{});check(!coverPreviewWindow&&textOf(GetDlgItem(panel,405)).find(L"recorte")!=std::wstring::npos,"controller confirms framed cover into editor draft");
 // Closing an editor must also close its preview and re-enable its owner.
 openCoverPreview(source);hide();check(!coverPreviewWindow&&!panel&&!IsWindowVisible(win),"launch/hide cleanup cannot leave orphaned cover preview");show();
 SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(win,screen);clearArt();games=oldGames;libraryView=oldView;windowPreset=oldPreset;applyWindowPreset(oldPreset);rebuildLibrary();saveDatabase();return failed;
}
