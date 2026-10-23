#include "app.h"
#include "display.h"
#include <cmath>
#include <cstring>

int windowPreset=0;
namespace {
 struct Child {HWND window;RECT rect;};
 struct Layout {int width=0,height=0;double scale=1;HFONT font=nullptr;std::vector<Child> children;};
 std::map<HWND,Layout> layouts;
 template<class T>T userApi(const char* name){auto p=GetProcAddress(GetModuleHandleW(L"user32.dll"),name);T result=nullptr;static_assert(sizeof(result)==sizeof(p),"Win32 ABI");memcpy(&result,&p,sizeof(p));return result;}
 int scaled(int value,double scale){return static_cast<int>(std::lround(value*scale));}
 void frame(RECT& rect,DWORD style,DWORD exStyle,UINT dpi){
  using Adjust=BOOL(WINAPI*)(LPRECT,DWORD,BOOL,DWORD,UINT);auto adjust=userApi<Adjust>("AdjustWindowRectExForDpi");
  if(adjust)adjust(&rect,style,FALSE,exStyle,dpi);else AdjustWindowRectEx(&rect,style,FALSE,exStyle);
 }
 void update(HWND owner){
  auto it=layouts.find(owner);if(it==layouts.end())return;auto& layout=it->second;
  RECT physical{};GetClientRect(owner,&physical);layout.scale=std::min(physical.right/static_cast<double>(layout.width),physical.bottom/static_cast<double>(layout.height));
  if(layout.scale<=0)return;
  auto old=layout.font;layout.font=CreateFontW(-std::max(10,scaled(15,layout.scale)),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
  for(const auto& child:layout.children)if(IsWindow(child.window)){const auto& r=child.rect;SetWindowPos(child.window,nullptr,scaled(r.left,layout.scale),scaled(r.top,layout.scale),scaled(r.right-r.left,layout.scale),scaled(r.bottom-r.top,layout.scale),SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(child.window,WM_SETFONT,reinterpret_cast<WPARAM>(layout.font),TRUE);}
  if(old)DeleteObject(old);
  RedrawWindow(owner,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
 }
}
void enablePerMonitorDpi(){
 using SetContext=BOOL(WINAPI*)(HANDLE);auto set=userApi<SetContext>("SetProcessDpiAwarenessContext");
 if(set&&set(reinterpret_cast<HANDLE>(-4)))return; // PER_MONITOR_AWARE_V2 (Windows 10+).
 HMODULE shcore=LoadLibraryW(L"shcore.dll");if(shcore){using SetAwareness=HRESULT(WINAPI*)(int);auto p=GetProcAddress(shcore,"SetProcessDpiAwareness");SetAwareness awareness=nullptr;memcpy(&awareness,&p,sizeof(p));if(awareness)awareness(2);FreeLibrary(shcore);}else SetProcessDPIAware();
}
UINT windowDpi(HWND owner){using GetDpi=UINT(WINAPI*)(HWND);auto get=userApi<GetDpi>("GetDpiForWindow");if(get&&owner){UINT dpi=get(owner);if(dpi)return dpi;}HDC dc=GetDC(owner);UINT dpi=dc?static_cast<UINT>(GetDeviceCaps(dc,LOGPIXELSX)):96;if(dc)ReleaseDC(owner,dc);return dpi?dpi:96;}
RECT monitorWorkArea(HWND owner){MONITORINFO info{};info.cbSize=sizeof(info);GetMonitorInfoW(MonitorFromWindow(owner,MONITOR_DEFAULTTONEAREST),&info);return info.rcWork;}
double uiScale(HWND owner){auto it=layouts.find(owner);return it==layouts.end()?windowDpi(owner)/96.0:it->second.scale;}
RECT logicalClient(HWND owner){auto it=layouts.find(owner);if(it!=layouts.end())return RECT{0,0,it->second.width,it->second.height};RECT r{};GetClientRect(owner,&r);const double scale=uiScale(owner);r.right=scaled(r.right,1/scale);r.bottom=scaled(r.bottom,1/scale);return r;}
int beginLogicalPaint(HWND owner,HDC dc){const int saved=SaveDC(dc);const double scale=uiScale(owner);SetMapMode(dc,MM_ANISOTROPIC);SetWindowExtEx(dc,10000,10000,nullptr);SetViewportExtEx(dc,scaled(10000,scale),scaled(10000,scale),nullptr);return saved;}
HWND createFittedWindow(const wchar_t* className,const wchar_t* title,int width,int height,HWND owner,DWORD style,DWORD exStyle){
 const RECT work=monitorWorkArea(owner);const UINT dpi=windowDpi(owner);RECT margin{};frame(margin,style,exStyle,dpi);
 const int availableW=work.right-work.left-16-(margin.right-margin.left),availableH=work.bottom-work.top-16-(margin.bottom-margin.top);
 const double scale=std::max(.25,std::min({dpi/96.0,availableW/static_cast<double>(width),availableH/static_cast<double>(height)}));
 RECT rect{0,0,scaled(width,scale),scaled(height,scale)};frame(rect,style,exStyle,dpi);const int w=rect.right-rect.left,h=rect.bottom-rect.top;
 RECT parent=work;if(owner)GetWindowRect(owner,&parent);
 const int x=std::clamp<LONG>(parent.left+(parent.right-parent.left-w)/2,work.left,std::max(work.left,work.right-w));
 const int y=std::clamp<LONG>(parent.top+(parent.bottom-parent.top-h)/2,work.top,std::max(work.top,work.bottom-h));
 HWND result=CreateWindowExW(exStyle,className,title,style|WS_CLIPCHILDREN,x,y,w,h,owner,nullptr,instance,nullptr);
 if(result){auto& layout=layouts[result];layout.width=width;layout.height=height;layout.scale=scale;update(result);darkTitle(result);}return result;
}
void registerLogicalChild(HWND owner,HWND child,RECT rect){auto it=layouts.find(owner);if(it==layouts.end()||!child)return;auto& layout=it->second;layout.children.push_back({child,rect});const double s=layout.scale;SetWindowPos(child,nullptr,scaled(rect.left,s),scaled(rect.top,s),scaled(rect.right-rect.left,s),scaled(rect.bottom-rect.top,s),SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(layout.font),TRUE);}
void forgetWindowLayout(HWND owner){auto it=layouts.find(owner);if(it!=layouts.end()){if(it->second.font)DeleteObject(it->second.font);layouts.erase(it);}}
bool handleLayoutMessage(HWND owner,UINT message,LPARAM parameter){
 if(message==WM_NCDESTROY){forgetWindowLayout(owner);return false;}
 if(message==WM_SIZE){update(owner);return false;}
 if(message!=WM_DPICHANGED)return false;
 RECT suggested=*reinterpret_cast<RECT*>(parameter);MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);GetMonitorInfoW(MonitorFromRect(&suggested,MONITOR_DEFAULTTONEAREST),&monitor);
 if(owner==win&&fullscreen){const auto& info=monitor;SetWindowPos(owner,nullptr,info.rcMonitor.left,info.rcMonitor.top,info.rcMonitor.right-info.rcMonitor.left,info.rcMonitor.bottom-info.rcMonitor.top,SWP_NOZORDER|SWP_NOACTIVATE);}
 else {RECT next=suggested,work=monitor.rcWork;int w=std::min(next.right-next.left,work.right-work.left),h=std::min(next.bottom-next.top,work.bottom-work.top);int x=std::clamp(next.left,work.left,std::max(work.left,work.right-w)),y=std::clamp(next.top,work.top,std::max(work.top,work.bottom-h));SetWindowPos(owner,nullptr,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE);}
 update(owner);InvalidateRect(owner,nullptr,FALSE);return true;
}
const wchar_t* windowPresetName(int preset){static const wchar_t* names[]={L"Automático — ajustar ao monitor",L"Compacto — 960 × 600",L"Notebook — 1280 × 720",L"Desktop — 1600 × 900",L"Full HD — 1920 × 1080",L"Maximizado"};return names[std::clamp(preset,0,5)];}
void mainMinimumSize(HWND owner,MINMAXINFO* info){RECT work=monitorWorkArea(owner);RECT r{0,0,800,560};frame(r,WS_OVERLAPPEDWINDOW,0,windowDpi(owner));info->ptMinTrackSize={std::min(r.right-r.left,work.right-work.left),std::min(r.bottom-r.top,work.bottom-work.top)};}
void applyWindowPreset(int preset){
 windowPreset=std::clamp(preset,0,5);if(fullscreen)leaveFullscreen();
 if(windowPreset==5){ShowWindow(win,SW_MAXIMIZE);return;}
 const RECT work=monitorWorkArea(win);const int availableW=work.right-work.left,availableH=work.bottom-work.top;
 static const SIZE sizes[]={{1180,780},{960,600},{1280,720},{1600,900},{1920,1080}};
 // Presets describe the complete outer window in physical pixels, capped to work area.
 const auto wanted=sizes[windowPreset];const int w=std::min<LONG>(wanted.cx,availableW),h=std::min<LONG>(wanted.cy,availableH);
 ShowWindow(win,SW_RESTORE);SetWindowPos(win,nullptr,work.left+(availableW-w)/2,work.top+(availableH-h)/2,w,h,SWP_NOZORDER|SWP_NOACTIVATE);InvalidateRect(win,nullptr,FALSE);
}
