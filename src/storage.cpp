#include "app.h"

std::wstring textOf(HWND h) {
 const int n=GetWindowTextLengthW(h);
 std::wstring s(static_cast<size_t>(n)+1,L'\0');
 GetWindowTextW(h,&s[0],n+1); s.resize(n); return s;
}
std::wstring readValue(const wchar_t* section,const wchar_t* key,const wchar_t* fallback) {
 wchar_t buffer[32768];
 GetPrivateProfileStringW(section,key,fallback,buffer,32768,ini.c_str());
 return buffer;
}
bool validField(const std::wstring& s,size_t max) {
 return s.size()<=max && s.find_first_of(L"\r\n\0",0,3)==std::wstring::npos;
}
void loadDatabase() {
 games.clear();
 controllerCompatibility=GetPrivateProfileIntW(L"Settings",L"ControllerCompatibility",0,ini.c_str())!=0;
 auto mode=GetPrivateProfileIntW(L"Settings",L"DefaultControllerMode",2,ini.c_str());
 defaultControllerMode=mode>=1&&mode<=3?static_cast<ControllerMode>(mode):ControllerMode::Xbox;
 preferredControllerGuid=readValue(L"Settings",L"PreferredController");
 openButton=GetPrivateProfileIntW(L"Settings",L"Open",13,ini.c_str());
 okButton=GetPrivateProfileIntW(L"Settings",L"Confirm",2,ini.c_str());
 backButton=GetPrivateProfileIntW(L"Settings",L"Back",3,ini.c_str());
 starButton=GetPrivateProfileIntW(L"Settings",L"Favorite",4,ini.c_str());
 autoOpen=GetPrivateProfileIntW(L"Settings",L"AutoOpen",0,ini.c_str())!=0;
 profileName=readValue(L"Profile",L"Name",L"Jogador");
 if(profileName.empty())profileName=L"Jogador";
 avatarPath=readValue(L"Profile",L"Avatar");
 const UINT count=std::min(2000u,GetPrivateProfileIntW(L"Games",L"Count",0,ini.c_str()));
 for(UINT i=0;i<count;++i) {
  const auto section=L"Game"+std::to_wstring(i);
  Game g;
  g.name=readValue(section.c_str(),L"Name"); g.path=readValue(section.c_str(),L"Path");
  g.args=readValue(section.c_str(),L"Args"); g.dir=readValue(section.c_str(),L"Dir");
  g.cover=readValue(section.c_str(),L"Cover"); g.note=readValue(section.c_str(),L"Note");
  const auto controller=GetPrivateProfileIntW(section.c_str(),L"ControllerMode",0,ini.c_str());
  g.controllerMode=controller<=3?static_cast<ControllerMode>(controller):ControllerMode::Auto;
  g.favorite=GetPrivateProfileIntW(section.c_str(),L"Favorite",0,ini.c_str())!=0;
  if(!g.name.empty()&&!g.path.empty())games.push_back(std::move(g));
 }
}
bool saveDatabase(HWND owner) {
 std::wstring s=L"[Settings]\r\nOpen="+std::to_wstring(openButton)+
  L"\r\nConfirm="+std::to_wstring(okButton)+L"\r\nBack="+std::to_wstring(backButton)+
  L"\r\nFavorite="+std::to_wstring(starButton)+L"\r\nAutoOpen="+(autoOpen?L"1":L"0")+L"\r\n";
 auto field=[&s](const wchar_t* key,const std::wstring& value) { s+=std::wstring(key)+L"=\""+value+L"\"\r\n"; };
 s+=L"ControllerCompatibility="+std::wstring(controllerCompatibility?L"1":L"0")+L"\r\nDefaultControllerMode="+std::to_wstring(static_cast<int>(defaultControllerMode))+L"\r\n";
 field(L"PreferredController",preferredControllerGuid);
 s+=L"[Profile]\r\n";field(L"Name",profileName);field(L"Avatar",avatarPath);
 s+=L"[Games]\r\nCount="+std::to_wstring(games.size())+L"\r\n";
 for(size_t i=0;i<games.size();++i) {
  const auto& g=games[i];s+=L"[Game"+std::to_wstring(i)+L"]\r\n";
  field(L"Name",g.name);field(L"Path",g.path);field(L"Args",g.args);field(L"Dir",g.dir);
  field(L"Cover",g.cover);field(L"Note",g.note);s+=L"ControllerMode="+std::to_wstring(static_cast<int>(g.controllerMode))+L"\r\n";s+=L"Favorite="+std::wstring(g.favorite?L"1":L"0")+L"\r\n";
 }
 const std::wstring tmp=ini+L".tmp";
 HANDLE file=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 bool good=false;
 if(file!=INVALID_HANDLE_VALUE) {
  WORD bom=0xfeff;DWORD written=0;const DWORD bytes=static_cast<DWORD>(s.size()*sizeof(wchar_t));
  good=WriteFile(file,&bom,2,&written,nullptr)&&written==2;
  good=good&&WriteFile(file,s.data(),bytes,&written,nullptr)&&written==bytes;
  good=good&&FlushFileBuffers(file);CloseHandle(file);
 }
 // A one-time backup preserves the original library before the v2 migration.
 if(good)CopyFileW(ini.c_str(),(ini+L".backup").c_str(),TRUE);
 if(!good||!MoveFileExW(tmp.c_str(),ini.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
  DeleteFileW(tmp.c_str());
  if(!selfTesting)MessageBoxW(owner?owner:win,L"Não foi possível salvar. Confira as permissões e o espaço em disco. Sua biblioteca anterior foi preservada.",L"FeatherPad",MB_ICONERROR);
  return false;
 }
 return true;
}
bool importArt(std::wstring& path,HWND owner) {
 if(path.empty())return true;
 if(!validateArt(path)) {
  MessageBoxW(owner,L"Escolha uma imagem PNG, JPG ou BMP válida, de até 2048 × 2048 pixels e 8 MB. A capa é reduzida automaticamente para economizar memória.",L"Não foi possível carregar a imagem",MB_ICONWARNING);
  return false;
 }
 const auto artFolder=dataFolder+L"\\art";
 CreateDirectoryW(artFolder.c_str(),nullptr);
 // Imported files already inside our managed directory need no duplicate copy.
 if(path.compare(0,artFolder.size()+1,artFolder+L"\\")==0)return true;
 GUID id{}; if(FAILED(CoCreateGuid(&id)))return false;
 wchar_t guid[40]{};StringFromGUID2(id,guid,40);
 const auto destination=artFolder+L"\\"+guid+L".png";
 if(!saveThumbnail(path,destination)) {
  DeleteFileW(destination.c_str());
  MessageBoxW(owner,L"Não foi possível importar a imagem. Confira as permissões da pasta do perfil.",L"FeatherPad",MB_ICONERROR);return false;
 }
 path=destination;return true;
}
bool startupEnabled() {
 HKEY key=nullptr;
 if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return false;
 const auto result=RegQueryValueExW(key,L"FeatherPad",nullptr,nullptr,nullptr,nullptr);
 RegCloseKey(key);return result==ERROR_SUCCESS;
}
void toggleStartup() {
 const bool enabled=startupEnabled();HKEY key=nullptr;
 if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS) {
  status(L"Não foi possível alterar a inicialização do Windows.");return;
 }
 LSTATUS result;
 if(enabled)result=RegDeleteValueW(key,L"FeatherPad");
 else {
  wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
  const auto cmd=L"\""+std::wstring(exe)+L"\" --background";
  result=RegSetValueExW(key,L"FeatherPad",0,REG_SZ,reinterpret_cast<const BYTE*>(cmd.c_str()),static_cast<DWORD>((cmd.size()+1)*sizeof(wchar_t)));
 }
 RegCloseKey(key);
 if(result!=ERROR_SUCCESS&&!selfTesting)MessageBoxW(panel,L"O Windows não permitiu alterar a inicialização.",L"FeatherPad",MB_ICONERROR);
}
void rebuildLibrary(int preferred) {
 visibleGames.clear();
 for(size_t i=0;i<games.size();++i)if((!favoritesOnly||games[i].favorite)&&gameMatchesSearch(games[i].name,searchQuery))visibleGames.push_back(static_cast<int>(i));
 if(preferred>=0)selection=preferred;
 if(std::find(visibleGames.begin(),visibleGames.end(),selection)==visibleGames.end())selection=visibleGames.empty()?-1:visibleGames.front();
 firstVisible=0;keyboardFocus=-1;
 if(win)InvalidateRect(win,nullptr,FALSE);
}
void moveSelection(int delta) {
 if(visibleGames.empty())return;
 auto it=std::find(visibleGames.begin(),visibleGames.end(),selection);
 const int count=static_cast<int>(visibleGames.size());
 const int pos=it==visibleGames.end()?0:static_cast<int>(it-visibleGames.begin());
 selection=visibleGames[(pos+delta%count+count)%count]; keyboardFocus=-1;
 InvalidateRect(win,nullptr,FALSE);
}
void setFilter(bool favorites) { favoritesOnly=favorites;rebuildLibrary(selection); }
void toggleFavorite() {
 if(selection<0||selection>=static_cast<int>(games.size()))return;
 auto& g=games[selection];g.favorite=!g.favorite;
 if(!saveDatabase())g.favorite=!g.favorite;
 rebuildLibrary(selection);
}
void launch() {
 if(selection<0||selection>=static_cast<int>(games.size()))return;
 const auto& g=games[selection];
 if(!bridgePrepareLaunch(g.controllerMode)) {
  if(selfTesting)return;
  if(MessageBoxW(win,(bridgeStatus()+L"\n\nA conversão para este jogo ainda não está pronta. Iniciar usando somente o controle físico?").c_str(),L"Compatibilidade do controle",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES)return;
 }
 SHELLEXECUTEINFOW s{};
 s.cbSize=sizeof(s);s.fMask=SEE_MASK_FLAG_NO_UI;s.hwnd=win;s.lpVerb=L"open";s.lpFile=g.path.c_str();
 s.lpParameters=g.args.empty()?nullptr:g.args.c_str();s.lpDirectory=g.dir.empty()?nullptr:g.dir.c_str();s.nShow=SW_SHOWNORMAL;
 if(ShellExecuteExW(&s))hide();
 else if(!selfTesting)MessageBoxW(win,(L"Não foi possível abrir o jogo (erro "+std::to_wstring(GetLastError())+L"). Confira o executável, os argumentos e a pasta de trabalho em Editar jogo.").c_str(),L"FeatherPad",MB_ICONERROR);
}
