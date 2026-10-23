#include "app.h"
#include "cover_preview.h"
#include <cmath>
HWND coverPreviewWindow=nullptr;
namespace {
 std::wstring sourcePath;CoverCrop framing;HWND owner=nullptr;
 enum {Import=810,Cancel,Left,Right,Up,Down,ZoomIn,ZoomOut,Reset};
 void rectangle(HDC dc,RECT r,COLORREF color){SetDCBrushColor(dc,color);FillRect(dc,&r,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));}
 void text(HDC dc,const std::wstring& value,RECT r,int size,COLORREF color){HFONT font=CreateFontW(-size,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);DrawTextW(dc,value.c_str(),-1,&r,DT_LEFT|DT_WORDBREAK);SelectObject(dc,old);DeleteObject(font);}
 void paint(HDC dc){
  rectangle(dc,RECT{0,0,740,570},RGB(12,17,25));text(dc,L"Enquadrar capa",RECT{24,18,700,54},24,RGB(239,245,250));
  text(dc,L"Confira o recorte 5:7 usado nas capas da biblioteca. Ajuste a posição e o zoom. A imagem original permanece intacta.",RECT{24,60,704,110},15,RGB(148,165,185));
  const auto* art=artFor(sourcePath);if(art){RECT crop=coverCropRect(art->width,art->height,framing);HDC image=CreateCompatibleDC(dc);auto old=SelectObject(image,art->bitmap);SetStretchBltMode(dc,HALFTONE);StretchBlt(dc,36,126,250,350,image,crop.left,crop.top,crop.right-crop.left,crop.bottom-crop.top,SRCCOPY);SelectObject(image,old);DeleteDC(image);}
  text(dc,L"Prévia da capa importada",RECT{36,484,312,510},14,RGB(100,229,193));
  text(dc,L"Posição e enquadramento",RECT{338,128,712,160},18,RGB(239,245,250));
  text(dc,L"Zoom: "+std::to_wstring(static_cast<int>(std::lround(framing.zoom*100)))+L"%\n\nControle: direcional escolhe uma ação; confirmar aplica. Voltar cancela.\n\nMouse: arraste sobre a prévia para deslocar o recorte. Use a roda para o zoom.",RECT{338,346,706,496},15,RGB(148,165,185));
 }
 void change(int command){switch(command){case Import:closeCoverPreview(true);return;case Cancel:closeCoverPreview(false);return;case Left:framing.x-=.05;break;case Right:framing.x+=.05;break;case Up:framing.y-=.05;break;case Down:framing.y+=.05;break;case ZoomIn:framing.zoom+=.1;break;case ZoomOut:framing.zoom-=.1;break;case Reset:framing={};break;}framing.x=std::clamp(framing.x,0.0,1.0);framing.y=std::clamp(framing.y,0.0,1.0);framing.zoom=std::clamp(framing.zoom,1.0,3.0);InvalidateRect(coverPreviewWindow,nullptr,FALSE);}
 POINT drag{};bool dragging=false;
 POINT logicalPoint(HWND h,LPARAM l){const auto scale=uiScale(h);return POINT{static_cast<LONG>(GET_X_LPARAM(l)/scale),static_cast<LONG>(GET_Y_LPARAM(l)/scale)};}
 LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
  if(handleLayoutMessage(h,m,l))return 0;
  switch(m){case WM_ERASEBKGND:return 1;case WM_PAINT:{PAINTSTRUCT ps{};auto dc=BeginPaint(h,&ps);int saved=beginLogicalPaint(h,dc);paint(dc);RestoreDC(dc,saved);EndPaint(h,&ps);return 0;}
  case WM_DRAWITEM:drawButton(reinterpret_cast<const DRAWITEMSTRUCT*>(l));return TRUE;
  case WM_COMMAND:if(HIWORD(w)==BN_CLICKED)change(LOWORD(w));return 0;
  case WM_LBUTTONDOWN:{drag=logicalPoint(h,l);RECT preview{36,126,286,476};dragging=PtInRect(&preview,drag)!=FALSE;if(dragging)SetCapture(h);return 0;}
  case WM_MOUSEMOVE:if(dragging){const auto p=logicalPoint(h,l);framing.x=std::clamp(framing.x-(p.x-drag.x)/250.0/framing.zoom,0.0,1.0);framing.y=std::clamp(framing.y-(p.y-drag.y)/350.0/framing.zoom,0.0,1.0);drag=p;InvalidateRect(h,nullptr,FALSE);}return 0;
  case WM_LBUTTONUP:if(dragging){dragging=false;ReleaseCapture();}return 0;
  case WM_CAPTURECHANGED:dragging=false;return 0;
  case WM_MOUSEWHEEL:change(GET_WHEEL_DELTA_WPARAM(w)>0?ZoomIn:ZoomOut);return 0;
  case WM_CLOSE:closeCoverPreview(false);return 0;}
  return DefWindowProcW(h,m,w,l);
 }
}
RECT coverCropRect(int w,int h,const CoverCrop& crop){
 const double zoom=std::clamp(crop.zoom,1.0,3.0);const double ratio=300.0/420.0;
 int cw=std::max(1,static_cast<int>(std::min(static_cast<double>(w),h*ratio)/zoom)),ch=std::max(1,static_cast<int>(std::min(static_cast<double>(h),w/ratio)/zoom));
 int left=static_cast<int>(std::clamp(crop.x,0.0,1.0)*(w-cw)),top=static_cast<int>(std::clamp(crop.y,0.0,1.0)*(h-ch));return RECT{left,top,left+cw,top+ch};
}
void openCoverPreview(const std::wstring& path){
 if(coverPreviewWindow||!validateArt(path))return;
 sourcePath=path;framing={};owner=panel?panel:win;
 WNDCLASSW wc{};wc.hInstance=instance;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.lpfnWndProc=proc;wc.lpszClassName=L"FeatherPadCoverPreview";RegisterClassW(&wc);
 coverPreviewWindow=createFittedWindow(wc.lpszClassName,L"Prévia da capa",740,570,owner);if(!coverPreviewWindow)return;
 auto button=[&](int id,const wchar_t* label,int x,int y,int width){HWND b=CreateWindowExW(0,L"BUTTON",label,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,x,y,width,38,coverPreviewWindow,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),instance,nullptr);registerLogicalChild(coverPreviewWindow,b,RECT{x,y,x+width,y+38});};
 button(Left,L"←",338,180,80);button(Up,L"↑",432,180,80);button(Right,L"→",526,180,80);button(Down,L"↓",620,180,80);
 button(ZoomOut,L"− Zoom",338,236,174);button(ZoomIn,L"+ Zoom",526,236,174);button(Reset,L"Centralizar / redefinir",338,292,362);
 button(Cancel,L"Cancelar",414,522,124);button(Import,L"Usar recorte",552,522,148);
 EnableWindow(owner,FALSE);ShowWindow(coverPreviewWindow,SW_SHOW);SetForegroundWindow(coverPreviewWindow);SetFocus(GetDlgItem(coverPreviewWindow,Import));
}
void closeCoverPreview(bool apply,bool activate){if(!coverPreviewWindow)return;if(apply)applyCoverPreview(sourcePath,framing);HWND closing=coverPreviewWindow;coverPreviewWindow=nullptr;EnableWindow(owner,TRUE);DestroyWindow(closing);sourcePath.clear();if(activate){SetForegroundWindow(owner);SetFocus(GetDlgItem(owner,A_BROWSE_COVER));}}
void coverPreviewDirection(int dx,int dy){if(!coverPreviewWindow)return;HWND next=GetNextDlgTabItem(coverPreviewWindow,GetFocus(),dx<0||dy<0);if(next)SetFocus(next);}
void coverPreviewConfirm(){HWND focus=GetFocus();if(coverPreviewWindow&&IsChild(coverPreviewWindow,focus))SendMessageW(focus,BM_CLICK,0,0);}
bool coverPreviewRoute(MSG& m){if(!coverPreviewWindow)return false;if(m.message==WM_KEYDOWN){if(m.wParam==VK_ESCAPE){closeCoverPreview(false);return true;}if(m.wParam==VK_RETURN||m.wParam==VK_SPACE){coverPreviewConfirm();return true;}if(m.wParam==VK_LEFT||m.wParam==VK_RIGHT||m.wParam==VK_UP||m.wParam==VK_DOWN){coverPreviewDirection(m.wParam==VK_LEFT?-1:m.wParam==VK_RIGHT?1:0,m.wParam==VK_UP?-1:m.wParam==VK_DOWN?1:0);return true;}}return IsDialogMessageW(coverPreviewWindow,&m)!=FALSE;}
