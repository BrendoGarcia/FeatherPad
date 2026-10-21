#include "app.h"
#include "experience.h"

HWND keyboardWindow=nullptr;
std::wstring searchQuery;
bool fullscreen=false;
namespace {
 HWND destination=nullptr,keyboardEdit=nullptr,keyboardOwner=nullptr;HFONT keyboardFont=nullptr;
 bool upper=false,symbols=false;int keyFocus=0;WINDOWPLACEMENT placement{};
 LONG_PTR oldStyle=0,oldExStyle=0;
 const std::wstring letters=L"qwertyuiopasdfghjklçzxcvbnm,.-0123456789@_:/\\\"'!?=áàâãéêíóôõ";
 const std::wstring symbolKeys=LR"KEYS(!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~áàâãäéèêëíìîïóòôõöúùûüñç€£ªº)KEYS";
 const wchar_t* commands[]={L"Apagar",L"Espaço",L"Maiúsculas",L"Limpar",L"Aplicar",L"Cancelar",L"← cursor",L"cursor →",L"Símbolos",L"Fim"};
 void refreshKeys(){for(int i=0;i<60;++i){wchar_t c=(symbols?symbolKeys:letters)[i];if(upper)CharUpperBuffW(&c,1);SetWindowTextW(GetDlgItem(keyboardWindow,700+i),std::wstring(1,c).c_str());}}
 void chooseKey(int index){keyFocus=std::max(0,std::min(69,index));SetFocus(GetDlgItem(keyboardWindow,700+keyFocus));}
 void press(int i){
  if(i==64){closeVirtualKeyboard(true);return;}if(i==65){closeVirtualKeyboard(false);return;}
  if(i==68){symbols=!symbols;refreshKeys();return;}
  if(i>=66){DWORD a=0,b=0;SendMessageW(keyboardEdit,EM_GETSEL,reinterpret_cast<WPARAM>(&a),reinterpret_cast<LPARAM>(&b));int pos=i==66?std::max(0,static_cast<int>(a)-1):i==67?std::min(GetWindowTextLengthW(keyboardEdit),static_cast<int>(b)+1):i==68?0:GetWindowTextLengthW(keyboardEdit);SendMessageW(keyboardEdit,EM_SETSEL,pos,pos);return;}
  if(i==62){upper=!upper;refreshKeys();return;}
  if(i==63){SetWindowTextW(keyboardEdit,L"");return;}
  if(i==60){DWORD a=0,b=0;SendMessageW(keyboardEdit,EM_GETSEL,reinterpret_cast<WPARAM>(&a),reinterpret_cast<LPARAM>(&b));if(a==b&&a>0)SendMessageW(keyboardEdit,EM_SETSEL,a-1,b);SendMessageW(keyboardEdit,EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(L""));return;}
  wchar_t c=i==61?L' ':(symbols?symbolKeys:letters)[i];if(upper)CharUpperBuffW(&c,1);
  const auto s=std::wstring(1,c);SendMessageW(keyboardEdit,EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(s.c_str()));
 }
 LRESULT CALLBACK keyboardProc(HWND h,UINT m,WPARAM w,LPARAM l){
  switch(m){
  case WM_ERASEBKGND:{RECT r{};GetClientRect(h,&r);HBRUSH b=CreateSolidBrush(RGB(16,23,33));FillRect(reinterpret_cast<HDC>(w),&r,b);DeleteObject(b);return 1;}
  case WM_DRAWITEM:drawButton(reinterpret_cast<const DRAWITEMSTRUCT*>(l));return TRUE;
  case WM_COMMAND:if(HIWORD(w)==BN_CLICKED&&LOWORD(w)>=700&&LOWORD(w)<770){press(LOWORD(w)-700);return 0;}break;
  case WM_CLOSE:closeVirtualKeyboard(false);return 0;
  }
  return DefWindowProcW(h,m,w,l);
 }
}
bool gameMatchesSearch(const std::wstring& name,const std::wstring& query){
 if(query.empty())return true;
 return FindNLSStringEx(LOCALE_NAME_USER_DEFAULT,NORM_IGNORECASE|NORM_IGNORENONSPACE|FIND_FROMSTART,name.c_str(),static_cast<int>(name.size()),query.c_str(),static_cast<int>(query.size()),nullptr,nullptr,nullptr,0)>=0;
}
void openVirtualKeyboard(HWND target){
 if(keyboardWindow){SetForegroundWindow(keyboardWindow);return;}
 destination=target;keyboardOwner=panel?panel:win;upper=false;symbols=false;keyFocus=0;
 WNDCLASSW c{};c.hInstance=instance;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.lpfnWndProc=keyboardProc;c.lpszClassName=L"FeatherPadKeyboard";RegisterClassW(&c);
 MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);GetMonitorInfoW(MonitorFromWindow(keyboardOwner,MONITOR_DEFAULTTONEAREST),&monitor);
 int width=std::min(1100L,monitor.rcWork.right-monitor.rcWork.left),height=std::min(680L,monitor.rcWork.bottom-monitor.rcWork.top);
 keyboardWindow=CreateWindowExW(WS_EX_DLGMODALFRAME,c.lpszClassName,target?L"Teclado virtual — editar texto":L"Pesquisar jogos",WS_CAPTION|WS_SYSMENU,
 monitor.rcWork.left+(monitor.rcWork.right-monitor.rcWork.left-width)/2,monitor.rcWork.top+(monitor.rcWork.bottom-monitor.rcWork.top-height)/2,width,height,keyboardOwner,nullptr,instance,nullptr);
 if(!keyboardWindow)return;
 RECT area{};GetClientRect(keyboardWindow,&area);const int cw=area.right,ch=area.bottom;
 keyboardFont=CreateFontW(-(width>=1000?20:16),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 auto add=[&](const wchar_t* cls,const std::wstring& text,DWORD style,int x,int y,int w,int height,int id){HWND h=CreateWindowExW(0,cls,text.c_str(),WS_CHILD|WS_VISIBLE|style,x,y,w,height,keyboardWindow,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),instance,nullptr);SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(keyboardFont?keyboardFont:GetStockObject(DEFAULT_GUI_FONT)),TRUE);return h;};
 keyboardEdit=add(L"EDIT",target?textOf(target):searchQuery,WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,16,18,cw-32,34,690);
 auto limit=target?SendMessageW(target,EM_GETLIMITTEXT,0,0):160;SendMessageW(keyboardEdit,EM_SETLIMITTEXT,limit,0);SendMessageW(keyboardEdit,EM_SETSEL,static_cast<WPARAM>(-1),-1);
 add(L"STATIC",L"Direcional: navegar  |  Confirmar: digitar  |  Voltar: cancelar. Aplicar salva o texto.",0,16,60,cw-32,30,691);
 const int kw=(cw-32)/10,kh=(ch-108)/7;
 for(int i=0;i<60;++i)add(L"BUTTON",std::wstring(1,letters[i]),WS_TABSTOP|BS_OWNERDRAW,16+(i%10)*kw,96+(i/10)*kh,kw-4,kh-4,700+i);
 const int cmdWidth=(cw-32)/10;
 for(int i=0;i<10;++i)add(L"BUTTON",commands[i],WS_TABSTOP|BS_OWNERDRAW,16+i*cmdWidth,96+6*kh,cmdWidth-4,kh-4,760+i);
 darkTitle(keyboardWindow);EnableWindow(keyboardOwner,FALSE);ShowWindow(keyboardWindow,SW_SHOW);SetForegroundWindow(keyboardWindow);chooseKey(0);
}
void closeVirtualKeyboard(bool apply){
 if(!keyboardWindow)return;
 if(apply){const auto value=textOf(keyboardEdit);if(destination&&IsWindow(destination))SetWindowTextW(destination,value.c_str());else{searchQuery=value;rebuildLibrary(selection);}}
 HWND closing=keyboardWindow;keyboardWindow=nullptr;EnableWindow(keyboardOwner,TRUE);DestroyWindow(closing);if(keyboardFont){DeleteObject(keyboardFont);keyboardFont=nullptr;}SetForegroundWindow(keyboardOwner);SetFocus(destination&&IsWindow(destination)?destination:keyboardOwner);destination=nullptr;
}
void keyboardDirection(int dx,int dy){
 if(!keyboardWindow)return;
 int id=GetDlgCtrlID(GetFocus())-700;if(id>=0&&id<70)keyFocus=id;
 int row=keyFocus/10,col=keyFocus%10;row=(row+dy+7)%7;col=(col+dx+10)%10;keyFocus=row*10+col;
 chooseKey(keyFocus);
}
void keyboardConfirm(){if(keyboardWindow){int id=GetDlgCtrlID(GetFocus())-700;if(id>=0&&id<70)keyFocus=id;press(keyFocus);}}
bool keyboardRoute(MSG& msg){
 if(!keyboardWindow||(msg.hwnd!=keyboardWindow&&!IsChild(keyboardWindow,msg.hwnd)))return false;
 if(msg.message==WM_KEYDOWN){if(msg.wParam==VK_ESCAPE){closeVirtualKeyboard(false);return true;}if(msg.wParam==VK_RETURN){if(GetFocus()==keyboardEdit)closeVirtualKeyboard(true);else keyboardConfirm();return true;}
 if(GetFocus()!=keyboardEdit){switch(msg.wParam){case VK_LEFT:keyboardDirection(-1,0);return true;case VK_RIGHT:keyboardDirection(1,0);return true;case VK_UP:keyboardDirection(0,-1);return true;case VK_DOWN:keyboardDirection(0,1);return true;}}}
 return IsDialogMessageW(keyboardWindow,&msg)!=FALSE;
}
void toggleFullscreen(){
 if(fullscreen){leaveFullscreen();return;}
 placement.length=sizeof(placement);GetWindowPlacement(win,&placement);oldStyle=GetWindowLongPtrW(win,GWL_STYLE);oldExStyle=GetWindowLongPtrW(win,GWL_EXSTYLE);
 MONITORINFO m{};m.cbSize=sizeof(m);GetMonitorInfoW(MonitorFromWindow(win,MONITOR_DEFAULTTONEAREST),&m);fullscreen=true;
 SetWindowLongPtrW(win,GWL_STYLE,oldStyle&~WS_OVERLAPPEDWINDOW);SetWindowLongPtrW(win,GWL_EXSTYLE,oldExStyle&~(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE));
 SetWindowPos(win,HWND_TOP,m.rcMonitor.left,m.rcMonitor.top,m.rcMonitor.right-m.rcMonitor.left,m.rcMonitor.bottom-m.rcMonitor.top,SWP_FRAMECHANGED|SWP_SHOWWINDOW);clearArt();
}
void leaveFullscreen(){if(!fullscreen)return;fullscreen=false;SetWindowLongPtrW(win,GWL_STYLE,oldStyle);SetWindowLongPtrW(win,GWL_EXSTYLE,oldExStyle);SetWindowPlacement(win,&placement);SetWindowPos(win,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);InvalidateRect(win,nullptr,FALSE);}
POINT canvasPoint(POINT p){if(!fullscreen)return p;RECT r{};GetClientRect(win,&r);const double scale=std::min(r.right/1100.0,r.bottom/760.0);if(scale<=0)return p;return POINT{static_cast<LONG>((p.x-(r.right-1100*scale)/2)/scale),static_cast<LONG>((p.y-(r.bottom-760*scale)/2)/scale)};}
void paintCanvas(HDC dc,RECT r){
 if(!fullscreen){paintMain(dc,r);return;}
 HBRUSH bg=CreateSolidBrush(RGB(12,17,25));FillRect(dc,&r,bg);DeleteObject(bg);
 const double scale=std::min(r.right/1100.0,r.bottom/760.0);int saved=SaveDC(dc);SetMapMode(dc,MM_ANISOTROPIC);SetWindowExtEx(dc,1100,760,nullptr);SetViewportExtEx(dc,static_cast<int>(1100*scale),static_cast<int>(760*scale),nullptr);SetViewportOrgEx(dc,static_cast<int>((r.right-1100*scale)/2),static_cast<int>((r.bottom-760*scale)/2),nullptr);paintMain(dc,RECT{0,0,1100,760});RestoreDC(dc,saved);
}
