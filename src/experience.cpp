#include "app.h"
#include "experience.h"

HWND keyboardWindow=nullptr;
std::wstring searchQuery;
bool fullscreen=false;
namespace {
 HWND destination=nullptr,keyboardEdit=nullptr,keyboardOwner=nullptr;
 HFONT keyFont=nullptr,textFont=nullptr,titleFont=nullptr;
 bool upper=false;int layout=0,keyFocus=0,hoverKey=-1;
 WINDOWPLACEMENT placement{};LONG_PTR oldStyle=0,oldExStyle=0;
 const std::wstring layouts[]={L"qwertyuiopasdfghjklzxcvbnm",L"1234567890@#$%&*()-_+=/\\:;\"",L"áàâãäéèêëíìîïóòôõöúùûüñç€£",LR"KEYS(!"?'[]{}<>|~^`,.:;@#$%&*+-=\/)KEYS"};
 const std::vector<int> characters={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,20,21,22,23,24,25,26};
 wchar_t character(int code){
  if(code==27)return L',';
  if(code==28)return L'.';
  auto it=std::find(characters.begin(),characters.end(),code);
  if(it==characters.end())return 0;
  auto n=static_cast<size_t>(it-characters.begin());if(n>=layouts[layout].size())return 0;
  wchar_t c=layouts[layout][n];if(upper)CharUpperBuffW(&c,1);return c;
 }
 void chooseKey(int code){HWND h=GetDlgItem(keyboardWindow,700+code);if(h&&IsWindowEnabled(h)){keyFocus=code;SetFocus(h);}}
 void refreshKeys(){
  for(int code:characters){wchar_t c=character(code);HWND h=GetDlgItem(keyboardWindow,700+code);SetWindowTextW(h,c?std::wstring(1,c).c_str():L"");EnableWindow(h,c!=0);InvalidateRect(h,nullptr,FALSE);}
  const wchar_t* next[]={L"123",L"Áé",L"#+=",L"ABC"};SetWindowTextW(GetDlgItem(keyboardWindow,768),next[layout]);InvalidateRect(GetDlgItem(keyboardWindow,762),nullptr,FALSE);
 }
 void eraseSelection(){DWORD a=0,b=0;SendMessageW(keyboardEdit,EM_GETSEL,reinterpret_cast<WPARAM>(&a),reinterpret_cast<LPARAM>(&b));if(a==b&&a>0){auto text=textOf(keyboardEdit);--a;if(a>0&&text[a]>=0xdc00&&text[a]<=0xdfff&&text[a-1]>=0xd800&&text[a-1]<=0xdbff)--a;SendMessageW(keyboardEdit,EM_SETSEL,a,b);}SendMessageW(keyboardEdit,EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(L""));}
 void press(int code){
  if(code==64){closeVirtualKeyboard(true);return;}if(code==65){closeVirtualKeyboard(false);return;}
  if(code==68){layout=(layout+1)%4;refreshKeys();return;}
  if(code==62){upper=!upper;refreshKeys();return;}
  if(code==63){SetWindowTextW(keyboardEdit,L"");return;}
  if(code==60){eraseSelection();return;}
  if(code==66||code==67||code==69){DWORD a=0,b=0;SendMessageW(keyboardEdit,EM_GETSEL,reinterpret_cast<WPARAM>(&a),reinterpret_cast<LPARAM>(&b));auto text=textOf(keyboardEdit);int pos=code==66?std::max(0,static_cast<int>(a)-1):code==67?std::min(static_cast<int>(text.size()),static_cast<int>(b)+1):static_cast<int>(text.size());if(pos>0&&pos<static_cast<int>(text.size())&&text[pos]>=0xdc00&&text[pos]<=0xdfff)pos+=code==66?-1:1;SendMessageW(keyboardEdit,EM_SETSEL,pos,pos);return;}
  wchar_t c=code==61?L' ':character(code);if(c){auto s=std::wstring(1,c);SendMessageW(keyboardEdit,EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(s.c_str()));}
 }
 void background(HDC dc){
  RECT r{};GetClientRect(keyboardWindow,&r);HBRUSH b=CreateSolidBrush(RGB(12,15,20));FillRect(dc,&r,b);DeleteObject(b);
  auto old=SelectObject(dc,titleFont);SetTextColor(dc,RGB(239,245,250));SetBkMode(dc,TRANSPARENT);RECT title{28,18,r.right-180,56};DrawTextW(dc,destination?L"Editar texto":L"Pesquisar na biblioteca",-1,&title,DT_LEFT|DT_SINGLELINE|DT_VCENTER);
  SelectObject(dc,textFont);SetTextColor(dc,RGB(155,167,182));RECT help{28,r.bottom-32,r.right-28,r.bottom-8};DrawTextW(dc,L"Direcional: mover   ·   Confirmar: digitar   ·   Voltar: cancelar   ·   ↵: aplicar",-1,&help,DT_CENTER|DT_SINGLELINE|DT_VCENTER);SelectObject(dc,old);
 }
 void drawKey(const DRAWITEMSTRUCT* item){
  const int saved=SaveDC(item->hDC);const auto caption=textOf(item->hwndItem);
  HBRUSH backgroundBrush=CreateSolidBrush(RGB(12,15,20));FillRect(item->hDC,&item->rcItem,backgroundBrush);DeleteObject(backgroundBrush);
  const int code=item->CtlID-700;bool focused=(item->itemState&ODS_FOCUS)!=0,disabled=(item->itemState&ODS_DISABLED)!=0;
  bool primary=code==64,active=code==62&&upper,hovered=hoverKey==code;
  COLORREF color=disabled?RGB(22,26,32):(focused?RGB(100,229,193):primary||active?RGB(44,109,94):hovered?RGB(70,77,87):RGB(45,51,61));
  HBRUSH brush=CreateSolidBrush(color);HPEN pen=CreatePen(PS_SOLID,focused?3:1,focused?RGB(239,255,250):color);auto oldB=SelectObject(item->hDC,brush),oldP=SelectObject(item->hDC,pen);
  RECT r=item->rcItem;InflateRect(&r,-3,-3);RoundRect(item->hDC,r.left,r.top,r.right,r.bottom,24,24);
  SelectObject(item->hDC,oldB);SelectObject(item->hDC,oldP);DeleteObject(brush);DeleteObject(pen);
  auto old=SelectObject(item->hDC,code>=63&&code<=69&&code!=68?textFont:keyFont);SetBkMode(item->hDC,TRANSPARENT);SetTextColor(item->hDC,focused?RGB(8,36,32):RGB(245,247,250));
  if(code==60||code==62){
   const int cx=(r.left+r.right)/2,cy=(r.top+r.bottom)/2;HPEN icon=CreatePen(PS_SOLID,2,focused?RGB(8,36,32):RGB(245,247,250));auto previousPen=SelectObject(item->hDC,icon),previousBrush=SelectObject(item->hDC,GetStockObject(HOLLOW_BRUSH));
   if(code==60){POINT outline[]={{cx-25,cy},{cx-11,cy-16},{cx+25,cy-16},{cx+25,cy+16},{cx-11,cy+16},{cx-25,cy}};Polyline(item->hDC,outline,6);MoveToEx(item->hDC,cx-1,cy-7,nullptr);LineTo(item->hDC,cx+13,cy+7);MoveToEx(item->hDC,cx+13,cy-7,nullptr);LineTo(item->hDC,cx-1,cy+7);}
   else {POINT outline[]={{cx,cy-20},{cx+17,cy},{cx+7,cy},{cx+7,cy+18},{cx-7,cy+18},{cx-7,cy},{cx-17,cy},{cx,cy-20}};Polyline(item->hDC,outline,8);}
   SelectObject(item->hDC,previousBrush);SelectObject(item->hDC,previousPen);DeleteObject(icon);SelectObject(item->hDC,old);RestoreDC(item->hDC,saved);return;
  }
  SetTextAlign(item->hDC,TA_LEFT|TA_TOP|TA_NOUPDATECP);SIZE extent{};GetTextExtentPoint32W(item->hDC,caption.c_str(),static_cast<int>(caption.size()),&extent);
  TextOutW(item->hDC,(r.left+r.right-extent.cx)/2,(r.top+r.bottom-extent.cy)/2,caption.c_str(),static_cast<int>(caption.size()));SelectObject(item->hDC,old);RestoreDC(item->hDC,saved);
 }
 LRESULT CALLBACK keyProc(HWND h,UINT m,WPARAM w,LPARAM l){
  auto old=reinterpret_cast<WNDPROC>(GetPropW(h,L"KeyOriginal"));
  if(m==WM_SETFOCUS||m==WM_KILLFOCUS)InvalidateRect(h,nullptr,FALSE);
  if(m==WM_MOUSEMOVE){const int code=GetDlgCtrlID(h)-700;if(hoverKey!=code){HWND previous=GetDlgItem(keyboardWindow,700+hoverKey);hoverKey=code;if(previous)InvalidateRect(previous,nullptr,FALSE);InvalidateRect(h,nullptr,FALSE);}TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT),TME_LEAVE,h,0};TrackMouseEvent(&track);}
  if(m==WM_MOUSELEAVE){hoverKey=-1;InvalidateRect(h,nullptr,FALSE);}
  return CallWindowProcW(old,h,m,w,l);
 }
 LRESULT CALLBACK keyboardProc(HWND h,UINT m,WPARAM w,LPARAM l){
  switch(m){
  case WM_ERASEBKGND:return 1;
  case WM_PAINT:{PAINTSTRUCT ps{};HDC dc=BeginPaint(h,&ps);background(dc);EndPaint(h,&ps);return 0;}
  case WM_PRINTCLIENT:background(reinterpret_cast<HDC>(w));return 0;
  case WM_CTLCOLOREDIT:{HDC dc=reinterpret_cast<HDC>(w);SetTextColor(dc,RGB(240,245,250));SetBkColor(dc,RGB(27,33,43));static HBRUSH editBrush=CreateSolidBrush(RGB(27,33,43));return reinterpret_cast<LRESULT>(editBrush);}
  case WM_DRAWITEM:drawKey(reinterpret_cast<const DRAWITEMSTRUCT*>(l));return TRUE;
  case WM_COMMAND:if(HIWORD(w)==BN_CLICKED&&LOWORD(w)>=700&&LOWORD(w)<770){press(LOWORD(w)-700);return 0;}break;
  case WM_CLOSE:closeVirtualKeyboard(false);return 0;
  }
  return DefWindowProcW(h,m,w,l);
 }
}
bool gameMatchesSearch(const std::wstring& name,const std::wstring& query){if(query.empty())return true;return FindNLSStringEx(LOCALE_NAME_USER_DEFAULT,NORM_IGNORECASE|NORM_IGNORENONSPACE|FIND_FROMSTART,name.c_str(),static_cast<int>(name.size()),query.c_str(),static_cast<int>(query.size()),nullptr,nullptr,nullptr,0)>=0;}
void openVirtualKeyboard(HWND target){
 if(keyboardWindow){SetForegroundWindow(keyboardWindow);return;}
 destination=target;keyboardOwner=panel?panel:win;upper=false;layout=0;keyFocus=0;hoverKey=-1;
 WNDCLASSW c{};c.hInstance=instance;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.lpfnWndProc=keyboardProc;c.lpszClassName=L"FeatherPadKeyboard";RegisterClassW(&c);
 MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);GetMonitorInfoW(MonitorFromWindow(keyboardOwner,MONITOR_DEFAULTTONEAREST),&monitor);
 int width=std::min(1180L,monitor.rcWork.right-monitor.rcWork.left-32),height=std::min(680L,monitor.rcWork.bottom-monitor.rcWork.top-32);
 keyboardWindow=CreateWindowExW(WS_EX_DLGMODALFRAME,c.lpszClassName,L"Teclado virtual",WS_POPUP|WS_BORDER|WS_CLIPCHILDREN,monitor.rcWork.left+(monitor.rcWork.right-monitor.rcWork.left-width)/2,monitor.rcWork.top+(monitor.rcWork.bottom-monitor.rcWork.top-height)/2,width,height,keyboardOwner,nullptr,instance,nullptr);if(!keyboardWindow)return;
 auto font=[](int n,int weight){return CreateFontW(-n,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");};
 keyFont=font(width>=1000?38:28,FW_NORMAL);textFont=font(width>=1000?18:14,FW_NORMAL);titleFont=font(24,FW_SEMIBOLD);
 RECT area{};GetClientRect(keyboardWindow,&area);const int cw=area.right,ch=area.bottom;
 keyboardEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",(target?textOf(target):searchQuery).c_str(),WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,28,70,cw-56,48,keyboardWindow,reinterpret_cast<HMENU>(690),instance,nullptr);
 SendMessageW(keyboardEdit,WM_SETFONT,reinterpret_cast<WPARAM>(titleFont),TRUE);SendMessageW(keyboardEdit,EM_SETLIMITTEXT,target?SendMessageW(target,EM_GETLIMITTEXT,0,0):160,0);SendMessageW(keyboardEdit,EM_SETSEL,static_cast<WPARAM>(-1),-1);
 auto add=[&](int code,const std::wstring& label,int x,int y,int w,int h){HWND key=CreateWindowExW(0,L"BUTTON",label.c_str(),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,x,y,w,h,keyboardWindow,reinterpret_cast<HMENU>(static_cast<INT_PTR>(700+code)),instance,nullptr);SendMessageW(key,WM_SETFONT,reinterpret_cast<WPARAM>(keyFont),TRUE);SetPropW(key,L"KeyOriginal",reinterpret_cast<HANDLE>(SetWindowLongPtrW(key,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(keyProc))));};
 add(65,L"Fechar ×",cw-164,16,136,40);add(66,L"← cursor",28,130,104,36);add(67,L"cursor →",140,130,104,36);add(69,L"Fim",252,130,88,36);add(63,L"Limpar",348,130,104,36);
 const double unit=(cw-56)/10.0;const int top=186,step=(ch-top-42)/4;
 auto rowKey=[&](int code,const std::wstring& label,double x,double units,int row){add(code,label,28+static_cast<int>(x*unit),top+row*step,static_cast<int>(units*unit)-5,step-7);};
 for(int i=0;i<10;++i)rowKey(i,std::wstring(1,character(i)),i,1,0);
 for(int i=0;i<9;++i)rowKey(10+i,std::wstring(1,character(10+i)),0.5+i,1,1);
 rowKey(62,L"⇧",0,1.25,2);for(int i=0;i<7;++i)rowKey(20+i,std::wstring(1,character(20+i)),1.25+i,1,2);rowKey(60,L"⌫",8.25,1.75,2);
 rowKey(68,L"123",0,1.4,3);rowKey(27,L",",1.4,1,3);rowKey(61,L"espaço",2.4,5.2,3);rowKey(28,L".",7.6,1,3);rowKey(64,L"↵ Aplicar",8.6,1.4,3);
 EnableWindow(keyboardOwner,FALSE);ShowWindow(keyboardWindow,SW_SHOW);SetForegroundWindow(keyboardWindow);chooseKey(0);
}
bool keyboardGlyphsVisible(){
 if(!keyboardWindow)return false;
 HDC screen=GetDC(keyboardWindow),dc=CreateCompatibleDC(screen);HBITMAP bitmap=CreateCompatibleBitmap(screen,120,100);auto old=SelectObject(dc,bitmap);
 DRAWITEMSTRUCT item{};item.CtlID=701;item.hwndItem=GetDlgItem(keyboardWindow,701);item.hDC=dc;item.rcItem=RECT{0,0,120,100};drawKey(&item);int light=0;
 for(int y=20;y<80;++y)for(int x=15;x<105;++x){auto c=GetPixel(dc,x,y);if(GetRValue(c)>160&&GetGValue(c)>160&&GetBValue(c)>160)++light;}
 bool corners=GetPixel(dc,0,0)==RGB(12,15,20);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(keyboardWindow,screen);return light>20&&corners;
}
void closeVirtualKeyboard(bool apply){
 if(!keyboardWindow)return;
 if(apply){auto value=textOf(keyboardEdit);if(destination&&IsWindow(destination))SetWindowTextW(destination,value.c_str());else{searchQuery=value;rebuildLibrary(selection);}}
 HWND closing=keyboardWindow;keyboardWindow=nullptr;EnableWindow(keyboardOwner,TRUE);DestroyWindow(closing);for(HFONT f:{keyFont,textFont,titleFont})if(f)DeleteObject(f);keyFont=textFont=titleFont=nullptr;SetForegroundWindow(keyboardOwner);SetFocus(destination&&IsWindow(destination)?destination:keyboardOwner);destination=nullptr;
}
void keyboardDirection(int dx,int dy){
 if(!keyboardWindow)return;
 int code=GetDlgCtrlID(GetFocus())-700;if(code>=0&&code<70)keyFocus=code;
 HWND from=GetDlgItem(keyboardWindow,700+keyFocus);RECT r{};GetWindowRect(from,&r);const int cx=(r.left+r.right)/2,cy=(r.top+r.bottom)/2;
 long best=LONG_MAX;int target=-1;
 for(int i=0;i<70;++i){HWND key=GetDlgItem(keyboardWindow,700+i);if(!key||!IsWindowEnabled(key)||i==keyFocus)continue;RECT rect{};GetWindowRect(key,&rect);const int x=(rect.left+rect.right)/2-cx,y=(rect.top+rect.bottom)/2-cy;const int primary=dx?x*dx:y*dy,secondary=dx?abs(y):abs(x);if(primary<=3)continue;long score=primary+3L*secondary;if(score<best){best=score;target=i;}}
 if(target>=0)chooseKey(target);
}
void keyboardConfirm(){if(keyboardWindow){int code=GetDlgCtrlID(GetFocus())-700;if(code>=0&&code<70)keyFocus=code;press(keyFocus);}}
bool keyboardRoute(MSG& msg){
 if(!keyboardWindow||(msg.hwnd!=keyboardWindow&&!IsChild(keyboardWindow,msg.hwnd)))return false;
 if(msg.message==WM_CHAR&&GetFocus()!=keyboardEdit&&msg.wParam>=32){wchar_t c=static_cast<wchar_t>(msg.wParam);auto text=std::wstring(1,c);SendMessageW(keyboardEdit,EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(text.c_str()));return true;}
 if(msg.message==WM_KEYDOWN){if(msg.wParam==VK_ESCAPE){closeVirtualKeyboard(false);return true;}if(msg.wParam==VK_RETURN){closeVirtualKeyboard(GetDlgCtrlID(GetFocus())!=765);return true;}if(GetFocus()!=keyboardEdit){switch(msg.wParam){case VK_LEFT:keyboardDirection(-1,0);return true;case VK_RIGHT:keyboardDirection(1,0);return true;case VK_UP:keyboardDirection(0,-1);return true;case VK_DOWN:keyboardDirection(0,1);return true;case VK_BACK:eraseSelection();return true;case VK_SPACE:SendMessageW(keyboardEdit,EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(L" "));return true;}}}
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
