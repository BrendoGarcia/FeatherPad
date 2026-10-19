#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

constexpr UINT TRAY=WM_APP+1;
constexpr int LIST=100, SAVE=101, REMOVE=102, PLAY=103, BROWSE=104, NEWGAME=105;
constexpr int OPEN=200, EXITAPP=201, CAPOPEN=202, CAPOK=203, CAPBACK=204, STARTUP=205, AUTOOPEN=206, DIAG=207;
struct Game { std::wstring name,path,args,dir; };
struct Device { std::vector<BYTE> descriptor; std::set<USAGE> previous; std::vector<HIDP_VALUE_CAPS> values; int direction=0; ULONGLONG repeat=0; };
HWND win,list,fields[4],statusLabel; HFONT font; HINSTANCE instance;
std::vector<Game> games; std::map<HANDLE,Device> devices;
std::wstring ini; unsigned openButton=13,okButton=2,backButton=3; int capture=0; bool autoOpen=false,diagnostic=false;
HANDLE lockHandle=nullptr; NOTIFYICONDATAW tray{};
std::wstring textOf(HWND h) { int n=GetWindowTextLengthW(h); std::wstring s(n+1,L'\0'); GetWindowTextW(h,&s[0],n+1); s.resize(n); return s; }
void status(const std::wstring& s) { SetWindowTextW(statusLabel,s.c_str()); }
std::wstring read(const wchar_t* section,const wchar_t* key,const wchar_t* fallback=L"") { wchar_t b[32768]; GetPrivateProfileStringW(section,key,fallback,b,32768,ini.c_str()); return b; }
bool write(const wchar_t* section,const wchar_t* key,const std::wstring& s) { return WritePrivateProfileStringW(section,key,s.c_str(),ini.c_str())!=0; }
void settings() { if(!write(L"Settings",L"Open",std::to_wstring(openButton)) || !write(L"Settings",L"Confirm",std::to_wstring(okButton)) || !write(L"Settings",L"Back",std::to_wstring(backButton)) || !write(L"Settings",L"AutoOpen",autoOpen?L"1":L"0")) MessageBoxW(win,L"Nao foi possivel salvar as configuracoes.",L"FeatherPad",MB_ICONERROR); }
void show() { ShowWindow(win,SW_RESTORE); SetForegroundWindow(win); SetFocus(list); }
void hide() { ShowWindow(win,SW_HIDE); }
int selected() { return (int)SendMessageW(list,LB_GETCURSEL,0,0); }
void populate() { int i=selected(); if(i<0 || i>=(int)games.size()) return; const auto& g=games[i]; const std::wstring* a[]={&g.name,&g.path,&g.args,&g.dir}; for(int j=0;j<4;++j) SetWindowTextW(fields[j],a[j]->c_str()); }
void refresh(int index) { SendMessageW(list,LB_RESETCONTENT,0,0); for(auto& g:games) SendMessageW(list,LB_ADDSTRING,0,(LPARAM)g.name.c_str()); if(!games.empty()) { SendMessageW(list,LB_SETCURSEL,std::min(std::max(index,0),(int)games.size()-1),0); populate(); } }
bool saveGames() {
 // Write a complete Unicode INI to a sibling file and atomically replace the old file.
 std::wstring output=L"[Settings]\r\nOpen="+std::to_wstring(openButton)+L"\r\nConfirm="+std::to_wstring(okButton)+L"\r\nBack="+std::to_wstring(backButton)+L"\r\nAutoOpen="+(autoOpen?L"1":L"0")+L"\r\n[Games]\r\nCount="+std::to_wstring(games.size())+L"\r\n";
 for(size_t i=0;i<games.size();++i) { auto& g=games[i]; output+=L"[Game"+std::to_wstring(i)+L"]\r\nName=\""+g.name+L"\"\r\nPath=\""+g.path+L"\"\r\nArgs=\""+g.args+L"\"\r\nDir=\""+g.dir+L"\"\r\n"; }
 std::wstring tmp=ini+L".tmp"; HANDLE f=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr); bool good=false;
 if(f!=INVALID_HANDLE_VALUE) { DWORD n=0; WORD bom=0xfeff; good=WriteFile(f,&bom,2,&n,nullptr)&&n==2; DWORD bytes=(DWORD)(output.size()*sizeof(wchar_t)); good=good&&WriteFile(f,output.data(),bytes,&n,nullptr)&&n==bytes; good=good&&FlushFileBuffers(f); CloseHandle(f); }
 if(!good || !MoveFileExW(tmp.c_str(),ini.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) { DeleteFileW(tmp.c_str()); MessageBoxW(win,L"Falha ao salvar a biblioteca. Confira o acesso a pasta de configuracao.",L"FeatherPad",MB_ICONERROR); return false; }
 return true;
}
void launch() { int i=selected(); if(i<0 || i>=(int)games.size()) return; auto& g=games[i]; SHELLEXECUTEINFOW s{}; s.cbSize=sizeof(s); s.fMask=SEE_MASK_FLAG_NO_UI; s.hwnd=win; s.lpVerb=L"open"; s.lpFile=g.path.c_str(); s.lpParameters=g.args.empty()?nullptr:g.args.c_str(); s.lpDirectory=g.dir.empty()?nullptr:g.dir.c_str(); s.nShow=SW_SHOWNORMAL;
 if(ShellExecuteExW(&s)) hide(); else { DWORD e=GetLastError(); MessageBoxW(win,(L"Nao foi possivel abrir o jogo. Erro Windows: "+std::to_wstring(e)+L". Confira o caminho, argumentos e pasta de trabalho.").c_str(),L"FeatherPad",MB_ICONERROR); } }
void moveSelection(int delta) { int n=(int)games.size(); if(!n)return; int i=selected(); i=i<0?0:(i+delta+n)%n; SendMessageW(list,LB_SETCURSEL,i,0); populate(); }
bool startupEnabled() { HKEY k; if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&k)!=ERROR_SUCCESS)return false; auto r=RegQueryValueExW(k,L"FeatherPad",nullptr,nullptr,nullptr,nullptr); RegCloseKey(k); return r==ERROR_SUCCESS; }
void toggleStartup() { HKEY k; if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,nullptr,0,KEY_SET_VALUE|KEY_QUERY_VALUE,nullptr,&k,nullptr)!=ERROR_SUCCESS) { status(L"Falha ao acessar inicializacao do Windows."); return; } LSTATUS result;
 if(startupEnabled()) result=RegDeleteValueW(k,L"FeatherPad"); else { wchar_t path[32768]; GetModuleFileNameW(nullptr,path,32768); std::wstring cmd=L"\""+std::wstring(path)+L"\" --background"; result=RegSetValueExW(k,L"FeatherPad",0,REG_SZ,(const BYTE*)cmd.c_str(),(DWORD)((cmd.size()+1)*sizeof(wchar_t))); } RegCloseKey(k); status(result==ERROR_SUCCESS?L"Inicializacao atualizada.":L"Falha ao atualizar inicializacao."); }
void menu() { HMENU m=CreatePopupMenu(); AppendMenuW(m,MF_STRING,OPEN,L"Abrir launcher"); AppendMenuW(m,MF_STRING,CAPOPEN,L"Configurar botao de abertura..."); AppendMenuW(m,MF_STRING,CAPOK,L"Configurar botao de confirmar..."); AppendMenuW(m,MF_STRING,CAPBACK,L"Configurar botao de voltar..."); AppendMenuW(m,MF_STRING|(startupEnabled()?MF_CHECKED:0),STARTUP,L"Iniciar com Windows"); AppendMenuW(m,MF_STRING|(autoOpen?MF_CHECKED:0),AUTOOPEN,L"Abrir ao conectar controle"); AppendMenuW(m,MF_STRING|(diagnostic?MF_CHECKED:0),DIAG,L"Diagnostico dos botoes"); AppendMenuW(m,MF_SEPARATOR,0,nullptr); AppendMenuW(m,MF_STRING,EXITAPP,L"Encerrar monitor"); POINT p; GetCursorPos(&p); SetForegroundWindow(win); TrackPopupMenu(m,TPM_RIGHTBUTTON,p.x,p.y,0,win,nullptr); DestroyMenu(m); PostMessageW(win,WM_NULL,0,0); }
Device* device(HANDLE h) { auto it=devices.find(h); if(it!=devices.end())return &it->second; UINT n=0; if(GetRawInputDeviceInfoW(h,RIDI_PREPARSEDDATA,nullptr,&n)==(UINT)-1 || !n || n>1024*1024)return nullptr; Device d; d.descriptor.resize(n); if(GetRawInputDeviceInfoW(h,RIDI_PREPARSEDDATA,d.descriptor.data(),&n)==(UINT)-1)return nullptr; auto pp=(PHIDP_PREPARSED_DATA)d.descriptor.data(); HIDP_CAPS caps{}; if(HidP_GetCaps(pp,&caps)!=HIDP_STATUS_SUCCESS || caps.UsagePage!=1 || (caps.Usage!=4 && caps.Usage!=5))return nullptr;
 USHORT count=caps.NumberInputValueCaps; d.values.resize(count); if(count && HidP_GetValueCaps(HidP_Input,d.values.data(),&count,pp)!=HIDP_STATUS_SUCCESS)d.values.clear(); else d.values.resize(count);
 auto r=devices.emplace(h,std::move(d)); status(L"Controle HID detectado. Use Configuracoes > Diagnostico para conferir os botoes."); return &r.first->second; }
void report(HANDLE h,char* data,ULONG length) {
 Device* d=device(h); if(!d)return; auto pp=(PHIDP_PREPARSED_DATA)d->descriptor.data(); USAGE buttons[128]{}; ULONG count=128;
 auto result=HidP_GetUsages(HidP_Input,9,0,buttons,&count,pp,data,length); if(result!=HIDP_STATUS_SUCCESS)return;
 std::set<USAGE> now(buttons,buttons+count); bool consumed=false;
 if(diagnostic && IsWindowVisible(win)) { std::wstring msg=L"HID botoes:"; for(auto b:now)msg+=L" "+std::to_wstring(b); status(msg); }
 for(auto b:now) if(!d->previous.count(b)) {
 if(capture) { if(capture==CAPOPEN)openButton=b; if(capture==CAPOK)okButton=b; if(capture==CAPBACK)backButton=b; capture=0; settings(); status(L"Botao salvo: "+std::to_wstring(b)); consumed=true; break; }
 if(b==openButton) { if(IsWindowVisible(win))hide(); else show(); consumed=true; break; }
 if(IsWindowVisible(win) && GetForegroundWindow()==win) { if(b==okButton) { launch(); consumed=true; break; } if(b==backButton) { hide(); consumed=true; break; } }
 }
 d->previous=now;
 if(consumed || !IsWindowVisible(win) || GetForegroundWindow()!=win) { d->direction=0;return; }
 int direction=0;
 for(auto& v:d->values) { if(v.UsagePage!=1)continue; USAGE first=v.IsRange?v.Range.UsageMin:v.NotRange.Usage; USAGE last=v.IsRange?v.Range.UsageMax:first;
 for(USAGE u: { (USAGE)0x39,(USAGE)0x31 }) { if(u<first || u>last)continue; ULONG value=0; if(HidP_GetUsageValue(HidP_Input,1,v.LinkCollection,u,&value,pp,data,length)!=HIDP_STATUS_SUCCESS)continue;
 LONG signedValue=(LONG)value; if(v.LogicalMin<0 && v.BitSize>0 && v.BitSize<32 && (value&(1UL<<(v.BitSize-1)))) signedValue=(LONG)(value|(~0UL<<v.BitSize));
 if(u==0x39) { LONG hat=signedValue-v.LogicalMin; if(signedValue>=v.LogicalMin && signedValue<=v.LogicalMax) { if(hat==7||hat==0||hat==1)direction=-1; if(hat==3||hat==4||hat==5)direction=1; } }
 else if(!direction && v.LogicalMax>v.LogicalMin) { double normalized=(double(signedValue)-v.LogicalMin)/(double(v.LogicalMax)-v.LogicalMin); if(normalized<0.25)direction=-1; else if(normalized>0.75)direction=1; }
 }
 }
 auto tick=GetTickCount64(); if(direction && (direction!=d->direction || tick>=d->repeat)) { moveSelection(direction); d->repeat=tick+(direction!=d->direction?400:150); } d->direction=direction;
}
void raw(LPARAM l) { UINT n=0; if(GetRawInputData((HRAWINPUT)l,RID_INPUT,nullptr,&n,sizeof(RAWINPUTHEADER))==(UINT)-1 || n<sizeof(RAWINPUTHEADER) || n>1024*1024)return; std::vector<BYTE> b(n); if(GetRawInputData((HRAWINPUT)l,RID_INPUT,b.data(),&n,sizeof(RAWINPUTHEADER))!=n)return; auto r=(RAWINPUT*)b.data(); if(r->header.dwType!=RIM_TYPEHID)return;
 auto& hid=r->data.hid; size_t offset=(size_t)(hid.bRawData-b.data()); if(!hid.dwSizeHid || offset>n || (size_t)hid.dwCount>(n-offset)/hid.dwSizeHid)return; for(DWORD i=0;i<hid.dwCount;++i)report(r->header.hDevice,(char*)hid.bRawData+(size_t)i*hid.dwSizeHid,hid.dwSizeHid); }
HWND control(const wchar_t* type,const wchar_t* title,DWORD style,int x,int y,int w,int h,int id) { HWND c=CreateWindowExW(0,type,title,WS_CHILD|WS_VISIBLE|style,x,y,w,h,win,(HMENU)(INT_PTR)id,instance,nullptr); SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE); return c; }
void command(int id) {
 if(id==208)menu(); else if(id==OPEN)show(); else if(id==EXITAPP) { DestroyWindow(win); }
 else if(id==STARTUP)toggleStartup(); else if(id==AUTOOPEN) { autoOpen=!autoOpen;settings(); }
 else if(id==DIAG) { diagnostic=!diagnostic; show(); status(L"Pressione os botoes do controle; os numeros aparecem aqui."); }
 else if(id>=CAPOPEN && id<=CAPBACK) { capture=id;show();status(L"Solte os botoes e pressione o botao desejado. Esc cancela."); }
 else if(id==PLAY)launch(); else if(id==NEWGAME) { SendMessageW(list,LB_SETCURSEL,(WPARAM)-1,0); for(auto f:fields)SetWindowTextW(f,L""); SetFocus(fields[0]); }
 else if(id==BROWSE) { wchar_t file[32768]{}; OPENFILENAMEW o{}; o.lStructSize=sizeof(o);o.hwndOwner=win;o.lpstrFile=file;o.nMaxFile=32768;o.lpstrFilter=L"Jogos e atalhos\0*.exe;*.lnk;*.url\0Todos os arquivos\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
 if(GetOpenFileNameW(&o)) { SetWindowTextW(fields[1],file); std::wstring p=file; auto slash=p.find_last_of(L"\\/"); if(slash!=std::wstring::npos) { SetWindowTextW(fields[3],p.substr(0,slash).c_str()); if(textOf(fields[0]).empty())SetWindowTextW(fields[0],p.substr(slash+1).c_str()); } } }
 else if(id==SAVE) { Game g{textOf(fields[0]),textOf(fields[1]),textOf(fields[2]),textOf(fields[3])}; if(g.name.empty()||g.path.empty()) { status(L"Preencha nome e caminho do jogo.");return; } for(auto s:{g.name,g.path,g.args,g.dir}) if(s.find_first_of(L"\r\n")!=std::wstring::npos || s.size()>8000) { status(L"Remova quebras de linha. Limite: 8000 caracteres por campo.");return; } int i=selected(); if(i<0) { if(games.size()>=2000){status(L"Limite de 2000 jogos.");return;} games.push_back(g);i=(int)games.size()-1; } else games[i]=g;bool saved=saveGames();refresh(i);status(saved?L"Jogo salvo.":L"Alteracao apenas em memoria: falha ao salvar."); }
 else if(id==REMOVE) { int i=selected(); if(i>=0 && MessageBoxW(win,L"Remover este jogo da lista? O arquivo do jogo sera preservado.",L"FeatherPad",MB_YESNO|MB_ICONQUESTION)==IDYES) { games.erase(games.begin()+i);saveGames();refresh(i);if(games.empty())for(auto f:fields)SetWindowTextW(f,L""); } }
}
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l) { switch(m) {
 case WM_COMMAND: if(LOWORD(w)==LIST && HIWORD(w)==LBN_SELCHANGE)populate(); else if(LOWORD(w)==LIST && HIWORD(w)==LBN_DBLCLK)launch(); else command(LOWORD(w)); return 0;
 case WM_INPUT: raw(l); return DefWindowProcW(h,m,w,l);
 case WM_INPUT_DEVICE_CHANGE: if(w==GIDC_REMOVAL) { devices.erase((HANDLE)l);status(L"Controle desconectado."); } else if(w==GIDC_ARRIVAL) { if(device((HANDLE)l) && autoOpen)show(); } return 0;
 case WM_CLOSE: capture=0;hide();return 0;
 case TRAY: if(l==WM_LBUTTONUP)show(); if(l==WM_RBUTTONUP)menu();return 0;
 case WM_DESTROY: Shell_NotifyIconW(NIM_DELETE,&tray);PostQuitMessage(0);return 0;
 } return DefWindowProcW(h,m,w,l); }
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,LPWSTR args,int) {
 instance=h; lockHandle=CreateMutexW(nullptr,FALSE,L"Local\\FeatherPad.Singleton.v1"); if(!lockHandle)return 1; if(GetLastError()==ERROR_ALREADY_EXISTS) { HWND other=FindWindowW(L"FeatherPadWindow",nullptr); if(other){ShowWindow(other,SW_RESTORE);SetForegroundWindow(other);} CloseHandle(lockHandle);return 0; }
 PWSTR base=nullptr; if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&base)))return 1; std::wstring folder=std::wstring(base)+L"\\FeatherPad";CoTaskMemFree(base); if(!CreateDirectoryW(folder.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return 1;ini=folder+L"\\library.ini";
 openButton=GetPrivateProfileIntW(L"Settings",L"Open",13,ini.c_str());okButton=GetPrivateProfileIntW(L"Settings",L"Confirm",2,ini.c_str());backButton=GetPrivateProfileIntW(L"Settings",L"Back",3,ini.c_str());autoOpen=GetPrivateProfileIntW(L"Settings",L"AutoOpen",0,ini.c_str())!=0;
 int count=std::min(2000u,GetPrivateProfileIntW(L"Games",L"Count",0,ini.c_str()));for(int i=0;i<count;++i) { auto sec=L"Game"+std::to_wstring(i); Game g{read(sec.c_str(),L"Name"),read(sec.c_str(),L"Path"),read(sec.c_str(),L"Args"),read(sec.c_str(),L"Dir")};if(!g.name.empty()&&!g.path.empty())games.push_back(g); }
 WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=h;wc.lpszClassName=L"FeatherPadWindow";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(nullptr,IDI_APPLICATION);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&wc);
 HMENU bar=CreateMenu();AppendMenuW(bar,MF_STRING,208,L"Configuracoes");
 win=CreateWindowExW(0,wc.lpszClassName,L"FeatherPad - Launcher leve",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,820,600,nullptr,bar,h,nullptr); if(!win)return 1;
 font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);control(L"STATIC",L"Biblioteca de jogos",0,16,12,740,22,0);
 list=control(L"LISTBOX",L"",LBS_NOTIFY|WS_VSCROLL|WS_BORDER|WS_TABSTOP,16,38,760,205,LIST);
 const wchar_t* labels[]={L"Nome",L"Executavel / atalho / URI",L"Argumentos (opcional)",L"Pasta de trabalho (opcional)"};for(int i=0;i<4;++i) { control(L"STATIC",labels[i],0,16,257+i*40,190,20,0);fields[i]=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,210,253+i*40,566,26,300+i);SendMessageW(fields[i],EM_SETLIMITTEXT,8000,0); }
 const wchar_t* names[]={L"Novo",L"Procurar...",L"Salvar",L"Remover",L"Jogar",L"Configuracoes"};int ids[]={NEWGAME,BROWSE,SAVE,REMOVE,PLAY,208};for(int i=0;i<6;++i)control(L"BUTTON",names[i],WS_TABSTOP,16+i*126,425,120,32,ids[i]);
 statusLabel=control(L"STATIC",L"Direcional/analogico: navegar | X: jogar | Circulo: ocultar | Esc: ocultar",0,16,472,760,58,0);refresh(0);
 RAWINPUTDEVICE rid[2]={{1,4,RIDEV_INPUTSINK|RIDEV_DEVNOTIFY,win},{1,5,RIDEV_INPUTSINK|RIDEV_DEVNOTIFY,win}};
 if(!RegisterRawInputDevices(rid,2,sizeof(RAWINPUTDEVICE)))MessageBoxW(win,L"Falha ao registrar controles. Teclado e mouse continuam disponiveis.",L"FeatherPad",MB_ICONERROR);
 tray.cbSize=sizeof(tray);tray.hWnd=win;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=TRAY;tray.hIcon=wc.hIcon;lstrcpyW(tray.szTip,L"FeatherPad - clique para abrir");if(!Shell_NotifyIconW(NIM_ADD,&tray))status(L"Falha ao criar icone na bandeja.");
 // Enumerate already-connected devices without opening a second Bluetooth connection.
 UINT n=0;GetRawInputDeviceList(nullptr,&n,sizeof(RAWINPUTDEVICELIST));if(n && n<4096){std::vector<RAWINPUTDEVICELIST> ds(n);UINT got=GetRawInputDeviceList(ds.data(),&n,sizeof(RAWINPUTDEVICELIST));if(got!=(UINT)-1)for(UINT i=0;i<got;++i)if(ds[i].dwType==RIM_TYPEHID)device(ds[i].hDevice);}
 if(std::wstring(args).find(L"--background")==std::wstring::npos)show();
 MSG msg{};int result;while((result=GetMessageW(&msg,nullptr,0,0))>0) { if(msg.message==WM_COMMAND && LOWORD(msg.wParam)==208){menu();continue;} if(msg.message==WM_KEYDOWN && (msg.hwnd==win||IsChild(win,msg.hwnd))) { if(msg.wParam==VK_ESCAPE) { if(capture){capture=0;status(L"Captura cancelada.");}else hide();continue; } if(msg.hwnd==list&&msg.wParam==VK_RETURN){launch();continue;} }
 if(!IsDialogMessageW(win,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);} }
 CloseHandle(lockHandle);return result==-1?1:0;
}
