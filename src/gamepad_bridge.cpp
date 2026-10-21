#include "app.h"
#include "gamepad_bridge.h"
#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <ViGEm/Client.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <cstdlib>
#include <cstring>

bool controllerCompatibility=false;
ControllerMode defaultControllerMode=ControllerMode::Xbox;
std::wstring preferredControllerGuid;

namespace {
 struct SDLApi {
  HMODULE module=nullptr;
#define FP_SDL_FUNCTION(name) decltype(&name) name=nullptr
  FP_SDL_FUNCTION(SDL_SetMainReady);FP_SDL_FUNCTION(SDL_InitSubSystem);FP_SDL_FUNCTION(SDL_QuitSubSystem);FP_SDL_FUNCTION(SDL_SetHint);
  FP_SDL_FUNCTION(SDL_GetError);FP_SDL_FUNCTION(SDL_PeepEvents);
  FP_SDL_FUNCTION(SDL_GetGamepads);FP_SDL_FUNCTION(SDL_GetJoysticks);FP_SDL_FUNCTION(SDL_IsGamepad);FP_SDL_FUNCTION(SDL_free);
  FP_SDL_FUNCTION(SDL_OpenGamepad);FP_SDL_FUNCTION(SDL_CloseGamepad);FP_SDL_FUNCTION(SDL_GamepadConnected);
  FP_SDL_FUNCTION(SDL_GetGamepadName);FP_SDL_FUNCTION(SDL_GetGamepadNameForID);FP_SDL_FUNCTION(SDL_GetGamepadGUIDForID);FP_SDL_FUNCTION(SDL_GUIDToString);
  FP_SDL_FUNCTION(SDL_GetGamepadPathForID);FP_SDL_FUNCTION(SDL_GetGamepadPlayerIndexForID);
  FP_SDL_FUNCTION(SDL_GetGamepadTypeForID);FP_SDL_FUNCTION(SDL_GetGamepadVendorForID);FP_SDL_FUNCTION(SDL_GetGamepadProductForID);
  FP_SDL_FUNCTION(SDL_GetGamepadButton);FP_SDL_FUNCTION(SDL_GetGamepadAxis);FP_SDL_FUNCTION(SDL_GetGamepadPowerInfo);FP_SDL_FUNCTION(SDL_AddGamepadMappingsFromFile);
  FP_SDL_FUNCTION(SDL_AttachVirtualJoystick);FP_SDL_FUNCTION(SDL_DetachVirtualJoystick);
  FP_SDL_FUNCTION(SDL_GetGamepadJoystick);FP_SDL_FUNCTION(SDL_SetJoystickVirtualButton);FP_SDL_FUNCTION(SDL_SetJoystickVirtualAxis);
  FP_SDL_FUNCTION(SDL_UpdateGamepads);
#undef FP_SDL_FUNCTION
 } api;
 bool uiArmed=false;
 bool initialized=false,driverTried=false,targetAttached=false,waitRelease=true,targetAttempted=false;
 unsigned epoch=0;UINT timerInterval=0;
 SDL_Gamepad* source=nullptr;SDL_JoystickID sourceId=0;
 PVIGEM_CLIENT client=nullptr;PVIGEM_TARGET target=nullptr;
 ControllerMode activeMode=ControllerMode::Xbox,targetMode=ControllerMode::Native;
 ULONG virtualXboxSlot=0xffffffffUL;
 std::set<SDL_JoystickID> excluded;
 std::set<USAGE> previousButtons;
 struct ExtraPlayer { SDL_Gamepad* pad=nullptr; SDL_JoystickID id=0; PVIGEM_TARGET target=nullptr; bool attached=false,waiting=true; ULONG slot=0xffffffffUL; ControllerMode mode=ControllerMode::Native; XboxState xbox{}; PlayStationState sony{}; bool hasLast=false; std::set<USAGE> previous; };
 std::vector<ExtraPlayer> extraPlayers;
 std::vector<ControllerSource> sources;
 ULONGLONG lastScan=0;
 std::wstring message=L"Compatibilidade desativada. Os jogos recebem o controle físico.";
 XboxState lastXbox{};PlayStationState lastSony{};bool hasLast=false;
 std::wstring wide(const char* s) {
  if(!s)return L"";
  const int n=MultiByteToWideChar(CP_UTF8,0,s,-1,nullptr,0);if(n<=1)return L"";
  std::wstring out(static_cast<size_t>(n),L'\0');MultiByteToWideChar(CP_UTF8,0,s,-1,&out[0],n);out.resize(n-1);return out;
 }
 std::string narrow(const std::wstring& s) {
  const int n=WideCharToMultiByte(CP_UTF8,0,s.c_str(),static_cast<int>(s.size()),nullptr,0,nullptr,nullptr);
  std::string out(static_cast<size_t>(n),'\0');if(n)WideCharToMultiByte(CP_UTF8,0,s.c_str(),static_cast<int>(s.size()),&out[0],n,nullptr,nullptr);return out;
 }
 void stateMessage(const std::wstring& value) {
  if(message==value)return;
  message=value;refreshBridgePanel();
  if(win&&IsWindowVisible(win))InvalidateRect(win,nullptr,FALSE);
 }
 std::wstring exeDirectory() {
  wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);std::wstring path=exe;return path.substr(0,path.find_last_of(L"\\/"));
 }
 bool loadSDL() {
  if(api.module)return true;
  const auto file=exeDirectory()+L"\\SDL3.dll";
  api.module=LoadLibraryExW(file.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!api.module){stateMessage(L"Biblioteca de controles ausente: extraia também SDL3.dll ao lado do FeatherPad.exe.");return false;}
#define LOAD_SDL(name) {FARPROC symbol=GetProcAddress(api.module,#name);static_assert(sizeof(api.name)==sizeof(symbol),"Windows function pointer ABI");std::memcpy(&api.name,&symbol,sizeof(symbol));}if(!api.name){FreeLibrary(api.module);api={};stateMessage(L"SDL3.dll incompatível. Use a DLL incluída no mesmo build do launcher.");return false;}
  LOAD_SDL(SDL_SetMainReady);LOAD_SDL(SDL_InitSubSystem);LOAD_SDL(SDL_QuitSubSystem);LOAD_SDL(SDL_SetHint);LOAD_SDL(SDL_GetError);
  LOAD_SDL(SDL_PeepEvents);LOAD_SDL(SDL_GetGamepads);LOAD_SDL(SDL_GetJoysticks);LOAD_SDL(SDL_IsGamepad);LOAD_SDL(SDL_free);
  LOAD_SDL(SDL_OpenGamepad);LOAD_SDL(SDL_CloseGamepad);LOAD_SDL(SDL_GamepadConnected);LOAD_SDL(SDL_GetGamepadName);LOAD_SDL(SDL_GetGamepadNameForID);
  LOAD_SDL(SDL_GetGamepadGUIDForID);LOAD_SDL(SDL_GUIDToString);LOAD_SDL(SDL_GetGamepadPathForID);LOAD_SDL(SDL_GetGamepadPlayerIndexForID);
  LOAD_SDL(SDL_GetGamepadTypeForID);LOAD_SDL(SDL_GetGamepadVendorForID);LOAD_SDL(SDL_GetGamepadProductForID);LOAD_SDL(SDL_GetGamepadButton);LOAD_SDL(SDL_GetGamepadAxis);LOAD_SDL(SDL_GetGamepadPowerInfo);
  LOAD_SDL(SDL_AddGamepadMappingsFromFile);LOAD_SDL(SDL_AttachVirtualJoystick);LOAD_SDL(SDL_DetachVirtualJoystick);
  LOAD_SDL(SDL_GetGamepadJoystick);LOAD_SDL(SDL_SetJoystickVirtualButton);LOAD_SDL(SDL_SetJoystickVirtualAxis);LOAD_SDL(SDL_UpdateGamepads);
#undef LOAD_SDL
  return true;
 }
 bool startSDL() {
  if(initialized)return true;
  if(!loadSDL())return false;
  api.SDL_SetMainReady();
  api.SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
  if(!api.SDL_InitSubSystem(SDL_INIT_GAMEPAD)){stateMessage(L"Não foi possível iniciar a leitura de controles: "+wide(api.SDL_GetError()));api.SDL_QuitSubSystem(SDL_INIT_GAMEPAD);FreeLibrary(api.module);api={};return false;}
  initialized=true;
  const auto mappingFile=narrow(dataFolder+L"\\gamecontrollerdb.txt");
  if(GetFileAttributesW((dataFolder+L"\\gamecontrollerdb.txt").c_str())!=INVALID_FILE_ATTRIBUTES)api.SDL_AddGamepadMappingsFromFile(mappingFile.c_str());
  return true;
 }
 void destroyTarget() {
  if(target){if(targetAttached)vigem_target_remove(client,target);vigem_target_free(target);target=nullptr;}
  targetAttached=false;targetMode=ControllerMode::Native;hasLast=false;targetAttempted=false;
  // Keep the last virtual slot excluded until SDL observes its removal event.
 }
 void closeExtra(ExtraPlayer& p){if(p.pad&&api.module)api.SDL_CloseGamepad(p.pad);p.pad=nullptr;p.id=0;if(p.target){if(p.attached)vigem_target_remove(client,p.target);vigem_target_free(p.target);}p.target=nullptr;p.attached=false;p.hasLast=false;p.mode=ControllerMode::Native;p.waiting=true;}
 void closeDriver() {
  for(auto& p:extraPlayers)closeExtra(p);
  extraPlayers.clear();
  destroyTarget();if(client){vigem_disconnect(client);vigem_free(client);client=nullptr;}driverTried=false;
 }
 void retuneTimer(bool fast) {
  if(selfTesting)return;
  const UINT interval=fast?8:250;
  if(timerInterval!=interval){SetTimer(win,GAMEPAD_TIMER,interval,nullptr);timerInterval=interval;}
 }
 std::wstring driverError(VIGEM_ERROR error) {
  if(error==VIGEM_ERROR_BUS_NOT_FOUND)return L"Driver virtual não encontrado. Instale ViGEmBus 1.22.0 uma vez e clique em Verificar novamente.";
  if(error==VIGEM_ERROR_NO_FREE_SLOT)return L"Não há vaga para outro controle virtual. Encerre outros emuladores e tente novamente.";
  if(error==VIGEM_ERROR_BUS_VERSION_MISMATCH)return L"A versão do driver virtual é incompatível. Confira a instalação do ViGEmBus 1.22.0.";
  return L"O driver virtual não está disponível (código "+std::to_wstring(static_cast<unsigned>(error))+L"). Verifique a instalação e tente novamente.";
 }
 bool ensureDriver() {
  if(client)return true;
  if(driverTried)return false;
  driverTried=true;
  client=vigem_alloc();if(!client){stateMessage(L"Não foi possível alocar o controle virtual.");return false;}
  const auto result=vigem_connect(client);
  if(!VIGEM_SUCCESS(result)){vigem_free(client);client=nullptr;stateMessage(driverError(result));return false;}return true;
 }
 bool excludedDevice(SDL_JoystickID id) {
  if(id==sourceId&&source)return false;
  for(const auto& player:extraPlayers)if(id==player.id&&player.pad)return false;
  if(excluded.count(id))return true;
  const auto path=wide(api.SDL_GetGamepadPathForID(id));
  if(!path.empty()&&isVirtualBusPath(path)){excluded.insert(id);return true;}
  // XInput paths may be absent. SDL documents its player index as the native
  // XInput user index, allowing exclusion of this exact virtual Xbox slot.
  const int playerIndex=api.SDL_GetGamepadPlayerIndexForID(id);
  if(api.SDL_GetGamepadTypeForID(id)==SDL_GAMEPAD_TYPE_XBOX360){
   if(targetAttached&&targetMode==ControllerMode::Xbox&&virtualXboxSlot!=0xffffffffUL&&playerIndex==static_cast<int>(virtualXboxSlot)){excluded.insert(id);return true;}
   for(const auto& player:extraPlayers)if(player.attached&&player.mode==ControllerMode::Xbox&&player.slot!=0xffffffffUL&&playerIndex==static_cast<int>(player.slot)){excluded.insert(id);return true;}
  }
  return false;
 }
 void scanSources() {
  sources.clear();int count=0;SDL_JoystickID* ids=api.SDL_GetGamepads(&count);
  for(int i=0;ids&&i<count;++i) {
   if(excludedDevice(ids[i]))continue;
   char guid[33]{};api.SDL_GUIDToString(api.SDL_GetGamepadGUIDForID(ids[i]),guid,33);
   const auto devicePath=wide(api.SDL_GetGamepadPathForID(ids[i]));
   sources.push_back({ids[i],wide(api.SDL_GetGamepadNameForID(ids[i])),wide(guid),devicePath.empty()?wide(guid):devicePath});
  }
  if(ids)api.SDL_free(ids);
  std::sort(sources.begin(),sources.end(),[](const auto& a,const auto& b){return CompareStringOrdinal(a.key.c_str(),-1,b.key.c_str(),-1,TRUE)<0;});
  lastScan=GetTickCount64();
 }
 void chooseSource() {
  if(source&&api.SDL_GamepadConnected(source))return;
  if(source){api.SDL_CloseGamepad(source);source=nullptr;sourceId=0;previousButtons.clear();destroyTarget();waitRelease=true;}
  for(const auto& candidate:sources) {
   if(!preferredControllerGuid.empty()&&candidate.guid!=preferredControllerGuid)continue;
   for(auto it=extraPlayers.begin();it!=extraPlayers.end();){if(it->id==candidate.id){closeExtra(*it);it=extraPlayers.erase(it);}else ++it;}
   source=api.SDL_OpenGamepad(candidate.id);if(source){sourceId=candidate.id;waitRelease=true;uiArmed=false;break;}
  }
  retuneTimer(source!=nullptr);
  if(win&&IsWindowVisible(win))InvalidateRect(win,nullptr,FALSE);
 }
 PadState readState(SDL_Gamepad* g) {
  PadState out;
  const SDL_GamepadButton sdl[]={SDL_GAMEPAD_BUTTON_SOUTH,SDL_GAMEPAD_BUTTON_EAST,SDL_GAMEPAD_BUTTON_WEST,SDL_GAMEPAD_BUTTON_NORTH,SDL_GAMEPAD_BUTTON_BACK,SDL_GAMEPAD_BUTTON_GUIDE,SDL_GAMEPAD_BUTTON_START,SDL_GAMEPAD_BUTTON_LEFT_STICK,SDL_GAMEPAD_BUTTON_RIGHT_STICK,SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,SDL_GAMEPAD_BUTTON_DPAD_UP,SDL_GAMEPAD_BUTTON_DPAD_DOWN,SDL_GAMEPAD_BUTTON_DPAD_LEFT,SDL_GAMEPAD_BUTTON_DPAD_RIGHT,SDL_GAMEPAD_BUTTON_TOUCHPAD};
  for(unsigned i=0;i<16;++i)if(api.SDL_GetGamepadButton(g,sdl[i]))out.buttons|=1u<<i;
  out.lx=api.SDL_GetGamepadAxis(g,SDL_GAMEPAD_AXIS_LEFTX);out.ly=api.SDL_GetGamepadAxis(g,SDL_GAMEPAD_AXIS_LEFTY);
  out.rx=api.SDL_GetGamepadAxis(g,SDL_GAMEPAD_AXIS_RIGHTX);out.ry=api.SDL_GetGamepadAxis(g,SDL_GAMEPAD_AXIS_RIGHTY);
  out.lt=static_cast<std::uint8_t>(std::max(0,static_cast<int>(api.SDL_GetGamepadAxis(g,SDL_GAMEPAD_AXIS_LEFT_TRIGGER)))*255/32767);
  out.rt=static_cast<std::uint8_t>(std::max(0,static_cast<int>(api.SDL_GetGamepadAxis(g,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)))*255/32767);
  return out;
 }
 VIGEM_ERROR submit(const PadState& state) {
  if(!targetAttached)return VIGEM_ERROR_NONE;
  if(targetMode==ControllerMode::Xbox) {
   const auto s=toXbox(state);
   if(hasLast&&s.buttons==lastXbox.buttons&&s.lt==lastXbox.lt&&s.rt==lastXbox.rt&&s.lx==lastXbox.lx&&s.ly==lastXbox.ly&&s.rx==lastXbox.rx&&s.ry==lastXbox.ry)return VIGEM_ERROR_NONE;
   XUSB_REPORT report{};report.wButtons=s.buttons;report.bLeftTrigger=s.lt;report.bRightTrigger=s.rt;report.sThumbLX=s.lx;report.sThumbLY=s.ly;report.sThumbRX=s.rx;report.sThumbRY=s.ry;
   const auto result=vigem_target_x360_update(client,target,report);if(VIGEM_SUCCESS(result)){lastXbox=s;hasLast=true;}return result;
  }
  const auto s=toPlayStation(state);
  if(hasLast&&s.buttons==lastSony.buttons&&s.special==lastSony.special&&s.lt==lastSony.lt&&s.rt==lastSony.rt&&s.lx==lastSony.lx&&s.ly==lastSony.ly&&s.rx==lastSony.rx&&s.ry==lastSony.ry)return VIGEM_ERROR_NONE;
  DS4_REPORT report{};report.bThumbLX=s.lx;report.bThumbLY=s.ly;report.bThumbRX=s.rx;report.bThumbRY=s.ry;report.wButtons=s.buttons;report.bSpecial=s.special;report.bTriggerL=s.lt;report.bTriggerR=s.rt;
  const auto result=vigem_target_ds4_update(client,target,report);if(VIGEM_SUCCESS(result)){lastSony=s;hasLast=true;}return result;
 }
 bool uiVisible();
 void syncExtraPlayers(){
  for(auto it=extraPlayers.begin();it!=extraPlayers.end();){if(!api.SDL_GamepadConnected(it->pad)){closeExtra(*it);it=extraPlayers.erase(it);}else ++it;}
  for(const auto& candidate:sources){if(extraPlayers.size()>=3)break;if(source&&candidate.id==sourceId)continue;if(std::any_of(extraPlayers.begin(),extraPlayers.end(),[&](const auto&p){return p.id==candidate.id;}))continue;SDL_Gamepad* pad=api.SDL_OpenGamepad(candidate.id);if(pad){ExtraPlayer p;p.pad=pad;p.id=candidate.id;extraPlayers.push_back(p);}}
 }
 bool submitExtra(ExtraPlayer& p,const PadState& state){
  if(activeMode==ControllerMode::Native){if(p.target){if(p.attached)vigem_target_remove(client,p.target);vigem_target_free(p.target);p.target=nullptr;p.attached=false;}p.mode=ControllerMode::Native;return true;}
  if(!ensureDriver())return false;
  if(!p.target||p.mode!=activeMode){if(p.target){if(p.attached)vigem_target_remove(client,p.target);vigem_target_free(p.target);}p.target=activeMode==ControllerMode::Xbox?vigem_target_x360_alloc():vigem_target_ds4_alloc();p.attached=false;p.hasLast=false;p.mode=activeMode;if(!p.target)return false;auto result=vigem_target_add(client,p.target);if(!VIGEM_SUCCESS(result)){vigem_target_free(p.target);p.target=nullptr;stateMessage(driverError(result));return false;}p.attached=true;p.waiting=true;p.slot=0xffffffffUL;if(activeMode==ControllerMode::Xbox)vigem_target_x360_get_user_index(client,p.target,&p.slot);}
  if(activeMode==ControllerMode::Xbox){const auto s=toXbox(state);if(p.hasLast&&p.xbox.buttons==s.buttons&&p.xbox.lt==s.lt&&p.xbox.rt==s.rt&&p.xbox.lx==s.lx&&p.xbox.ly==s.ly&&p.xbox.rx==s.rx&&p.xbox.ry==s.ry)return true;XUSB_REPORT r{};r.wButtons=s.buttons;r.bLeftTrigger=s.lt;r.bRightTrigger=s.rt;r.sThumbLX=s.lx;r.sThumbLY=s.ly;r.sThumbRX=s.rx;r.sThumbRY=s.ry;auto result=vigem_target_x360_update(client,p.target,r);if(VIGEM_SUCCESS(result)){p.xbox=s;p.hasLast=true;}return VIGEM_SUCCESS(result);}
  const auto s=toPlayStation(state);if(p.hasLast&&p.sony.buttons==s.buttons&&p.sony.special==s.special&&p.sony.lt==s.lt&&p.sony.rt==s.rt&&p.sony.lx==s.lx&&p.sony.ly==s.ly&&p.sony.rx==s.rx&&p.sony.ry==s.ry)return true;DS4_REPORT r{};r.bThumbLX=s.lx;r.bThumbLY=s.ly;r.bThumbRX=s.rx;r.bThumbRY=s.ry;r.wButtons=s.buttons;r.bSpecial=s.special;r.bTriggerL=s.lt;r.bTriggerR=s.rt;auto result=vigem_target_ds4_update(client,p.target,r);if(VIGEM_SUCCESS(result)){p.sony=s;p.hasLast=true;}return VIGEM_SUCCESS(result);
 }
 void tickExtraPlayers(){if(!controllerCompatibility)return;syncExtraPlayers();for(auto& p:extraPlayers){auto it=std::find_if(sources.begin(),sources.end(),[&](const auto& v){return v.id==p.id;});useControllerProfile(it==sources.end()?L"":it->key);auto state=calibratePad(readState(p.pad),profileDeadzone());if((captureWin&&GetForegroundWindow()==captureWin)||(panel&&page==Page::Diagnostic&&GetForegroundWindow()==panel)){std::set<USAGE> buttons;const std::uint32_t positions[]={PadWest,PadSouth,PadEast,PadNorth,PadLShoulder,PadRShoulder,0,0,PadBack,PadStart,PadLStick,PadRStick,PadGuide,PadTouch};for(unsigned i=0;i<14;++i)if(positions[i]&&(state.buttons&positions[i]))buttons.insert(static_cast<USAGE>(i+1));if(state.lt>30)buttons.insert(7);if(state.rt>30)buttons.insert(8);const HANDLE handle=reinterpret_cast<HANDLE>(-static_cast<INT_PTR>(p.id)-2);const unsigned before=epoch;controllerButtons(handle,buttons,p.previous);if(before!=epoch)return;p.previous=std::move(buttons);}if(activeMode!=ControllerMode::Native){if(uiVisible())p.waiting=true;else if(p.waiting&&padAtRest(state))p.waiting=false;if(!submitExtra(p,uiVisible()||p.waiting?PadState{}:state))stateMessage(L"Uma saída de controle multiplayer falhou. Confira o driver virtual e as vagas disponíveis.");}}}
 bool ensureTarget() {
  if(activeMode==ControllerMode::Native){destroyTarget();stateMessage(source?L"Modo nativo: o jogo recebe o controle físico, sem conversão.":L"Modo nativo. Conecte um controle para usar o launcher.");return true;}
  if(!ensureDriver())return false;
  if(!source){stateMessage(L"Driver disponível. Conecte um controle reconhecido ou selecione a origem.");return false;}
  if(targetAttached&&targetMode==activeMode)return true;
  if(targetAttempted)return false;
  destroyTarget();targetAttempted=true;target=activeMode==ControllerMode::Xbox?vigem_target_x360_alloc():vigem_target_ds4_alloc();
  if(!target){stateMessage(L"Não foi possível criar o controle virtual.");return false;}
  const auto result=vigem_target_add(client,target);
  if(!VIGEM_SUCCESS(result)){vigem_target_free(target);target=nullptr;stateMessage(driverError(result));return false;}
  targetAttached=true;targetMode=activeMode;virtualXboxSlot=0xffffffffUL;
  if(targetMode==ControllerMode::Xbox)vigem_target_x360_get_user_index(client,target,&virtualXboxSlot);
  hasLast=false;waitRelease=true;submit(PadState{});
  stateMessage(std::wstring(activeMode==ControllerMode::Xbox?L"Xbox 360 virtual ativo — ":L"PlayStation 4 virtual ativo — ")+wide(api.SDL_GetGamepadName(source))+L". Pronto para jogos compatíveis com esse formato.");
  return true;
 }
 bool uiVisible() {return keyboardWindow||panel||captureWin||(win&&IsWindowVisible(win)&&!IsIconic(win));}
 void routeUI(const PadState& s) {
  const auto it=std::find_if(sources.begin(),sources.end(),[](const auto& item){return item.id==sourceId;});
  useControllerProfile(it==sources.end()?L"":it->key);
  std::set<USAGE> buttons;
  const std::uint32_t positions[]={PadWest,PadSouth,PadEast,PadNorth,PadLShoulder,PadRShoulder,0,0,PadBack,PadStart,PadLStick,PadRStick,PadGuide,PadTouch};
  for(unsigned i=0;i<14;++i)if(positions[i]&&(s.buttons&positions[i]))buttons.insert(static_cast<USAGE>(i+1));
  if(s.lt>30)buttons.insert(7);
  if(s.rt>30)buttons.insert(8);
  const auto prior=previousButtons;previousButtons=buttons;
  const HANDLE handle=reinterpret_cast<HANDLE>(-static_cast<INT_PTR>(sourceId)-2);
  controllerButtons(handle,buttons,prior);
 }
}

bool isVirtualBusPath(const std::wstring& path) {
 HDEVINFO set=SetupDiCreateDeviceInfoList(nullptr,nullptr);if(set==INVALID_HANDLE_VALUE)return false;
 SP_DEVICE_INTERFACE_DATA iface{};iface.cbSize=sizeof(iface);bool virtualBus=false;
 if(SetupDiOpenDeviceInterfaceW(set,path.c_str(),0,&iface)) {
  DWORD size=0;SetupDiGetDeviceInterfaceDetailW(set,&iface,nullptr,0,&size,nullptr);
  if(size>=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)&&size<65536) {
   std::vector<BYTE> data(size);auto detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(data.data());detail->cbSize=sizeof(*detail);
   SP_DEVINFO_DATA info{};info.cbSize=sizeof(info);
   if(SetupDiGetDeviceInterfaceDetailW(set,&iface,detail,size,nullptr,&info)) {
    DEVINST node=info.DevInst;
    for(int depth=0;depth<12;++depth) {
     wchar_t id[MAX_DEVICE_ID_LEN]{};
     if(CM_Get_Device_IDW(node,id,MAX_DEVICE_ID_LEN,0)==CR_SUCCESS){CharUpperBuffW(id,lstrlenW(id));if(std::wstring(id).find(L"VIGEM")!=std::wstring::npos){virtualBus=true;break;}}
     DEVINST parent=0;if(CM_Get_Parent(&parent,node,0)!=CR_SUCCESS)break;node=parent;
    }
   }
  }
 }
 SetupDiDestroyDeviceInfoList(set);return virtualBus;
}
void bridgeShutdown() {
 uiArmed=false;++epoch;if(win)KillTimer(win,GAMEPAD_TIMER);timerInterval=0;closeDriver();
 if(source&&api.module)api.SDL_CloseGamepad(source);
 source=nullptr;sourceId=0;
 if(initialized&&api.module)api.SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
 initialized=false;
 if(api.module)FreeLibrary(api.module);
 api={};sources.clear();excluded.clear();previousButtons.clear();virtualXboxSlot=0xffffffffUL;lastScan=0;waitRelease=true;
}
void bridgeReconfigure() {
 bridgeShutdown();activeMode=resolveControllerMode(ControllerMode::Auto,defaultControllerMode);
 if(!controllerCompatibility){stateMessage(L"Compatibilidade desativada. Os jogos recebem o controle físico.");return;}
 if(selfTesting)return;
 if(!startSDL())return;
 scanSources();chooseSource();ensureTarget();retuneTimer(source!=nullptr);refreshBridgePanel();
}
bool bridgeOwnsUI(){return controllerCompatibility&&initialized&&source!=nullptr;}
void bridgeSuspendForUI(){waitRelease=true;if(targetAttached)submit(PadState{});}
void bridgeTick() {
 if(!controllerCompatibility||!initialized)return;
 const auto started=epoch;
 // Win32 already dispatches window messages. Update input directly and drain
 // SDL's queue without SDL_PollEvent auto-pumping/re-entering our WM_TIMER.
 api.SDL_UpdateGamepads();SDL_Event event{};bool rescan=false;
 while(api.SDL_PeepEvents(&event,1,SDL_GETEVENT,SDL_EVENT_FIRST,SDL_EVENT_LAST)>0) {
  if(event.type==SDL_EVENT_GAMEPAD_ADDED||event.type==SDL_EVENT_GAMEPAD_REMOVED)rescan=true;
  if(event.type==SDL_EVENT_GAMEPAD_REMOVED)excluded.erase(event.gdevice.which);
 }
 if(rescan||GetTickCount64()-lastScan>=500){scanSources();chooseSource();refreshBridgePanel();}
 if(source&&!api.SDL_GamepadConnected(source)){scanSources();chooseSource();refreshBridgePanel();}
 if(!source){ensureTarget();return;}
 if(!targetAttached&&activeMode!=ControllerMode::Native&&!driverTried)ensureTarget();
 // Reconnect can select a new physical source while retaining a valid bus client.
 if(!targetAttached&&client&&activeMode!=ControllerMode::Native)ensureTarget();
 const auto mainProfile=std::find_if(sources.begin(),sources.end(),[](const auto& v){return v.id==sourceId;});useControllerProfile(mainProfile==sources.end()?L"":mainProfile->key);
 const PadState state=calibratePad(readState(source),profileDeadzone());if(!uiArmed&&padAtRest(state))uiArmed=true;if(uiArmed)routeUI(state);if(started!=epoch||!source)return;
 const HWND foreground=GetForegroundWindow();
 if(uiArmed&&!captureWin&&(foreground==win||foreground==panel||foreground==keyboardWindow)) {
  static int lastDirection=0;static ULONGLONG nextDirection=0;int direction=0;
  if(state.buttons&PadLeft)direction=-1;else if(state.buttons&PadRight)direction=1;else if(state.buttons&PadUp)direction=-2;else if(state.buttons&PadDown)direction=2;
  else {const int dz=profileDeadzone();if(state.lx<-dz)direction=-1;else if(state.lx>dz)direction=1;else if(state.ly<-dz)direction=-2;else if(state.ly>dz)direction=2;}
  const auto tick=GetTickCount64();if(direction&&(direction!=lastDirection||tick>=nextDirection)){controllerDirection(abs(direction)==1?direction:0,abs(direction)==2?direction/2:0);nextDirection=tick+(direction!=lastDirection?400:150);}lastDirection=direction;
 }
 if(uiVisible())waitRelease=true;
 else if(waitRelease&&padAtRest(state))waitRelease=false;
 const auto result=submit(uiVisible()||waitRelease?PadState{}:state);tickExtraPlayers();
 if(!VIGEM_SUCCESS(result)){stateMessage(driverError(result));closeDriver();driverTried=true;}
}
bool bridgePrepareLaunch(ControllerMode requested) {
 if(!controllerCompatibility)return true;
 const auto desired=resolveControllerMode(requested,defaultControllerMode);
 if(desired!=activeMode){destroyTarget();for(auto& p:extraPlayers)if(p.target){if(p.attached)vigem_target_remove(client,p.target);vigem_target_free(p.target);p.target=nullptr;p.attached=false;p.hasLast=false;p.mode=ControllerMode::Native;}activeMode=desired;}
 if(!initialized&&!selfTesting){if(!startSDL())return false;scanSources();chooseSource();retuneTimer(source!=nullptr);}
 const bool ready=ensureTarget();
 bridgeSuspendForUI();return ready;
}
std::wstring bridgeStatus(){return message;}
size_t bridgePlayerCount(){return std::min<size_t>(4,sources.empty()?devices.size():sources.size());}
std::wstring bridgePlayerSummary(){std::wstring s=L"Controles: "+std::to_wstring(bridgePlayerCount())+L"/4";if(targetAttached&&targetMode==ControllerMode::Xbox&&virtualXboxSlot!=0xffffffffUL)s+=L" · P1 XInput "+std::to_wstring(virtualXboxSlot+1);unsigned number=2;for(const auto& p:extraPlayers){if(p.attached&&p.mode==ControllerMode::Xbox&&p.slot!=0xffffffffUL)s+=L" · P"+std::to_wstring(number)+L" XInput "+std::to_wstring(p.slot+1);++number;}return s;}
std::vector<std::wstring> bridgePlayerRows(){
 std::vector<std::wstring> rows;std::set<unsigned> shown;
 auto append=[&](unsigned id,const std::wstring& name,ControllerMode mode,bool attached,ULONG slot){
  if(rows.size()>=4)return;
  const auto output=mode==ControllerMode::Native?L"Físico / nativo":!attached?L"Saída virtual indisponível":mode==ControllerMode::Xbox?(slot==0xffffffffUL?std::wstring(L"Xbox · índice indisponível"):L"Xbox · posição XInput "+std::to_wstring(slot+1)):L"PlayStation 4 virtual";
  rows.push_back(L"P"+std::to_wstring(rows.size()+1)+L" · "+name+L"\r\nConectado · "+output);shown.insert(id);
 };
 if(source){const auto it=std::find_if(sources.begin(),sources.end(),[](const auto& s){return s.id==sourceId;});append(sourceId,it==sources.end()?L"Controle físico":it->name,activeMode,targetAttached,virtualXboxSlot);}
 for(const auto& player:extraPlayers){const auto it=std::find_if(sources.begin(),sources.end(),[&](const auto& s){return s.id==player.id;});append(player.id,it==sources.end()?L"Controle físico":it->name,activeMode,player.attached,player.slot);}
 for(const auto& item:sources)if(!shown.count(item.id))append(item.id,item.name,activeMode,false,0xffffffffUL);
 if(rows.empty())for(const auto& item:devices){if(rows.size()>=4)break;rows.push_back(L"P"+std::to_wstring(rows.size()+1)+L" · Controle físico\r\nConectado · modo nativo");(void)item;}
 return rows;
}
std::wstring bridgeBatteryStatus(){if(!api.SDL_GetGamepadPowerInfo||sources.empty())return L"Bateria: indisponível";auto value=[&](SDL_Gamepad* pad){if(!pad)return std::wstring(L"n/d");int percent=-1;const auto power=api.SDL_GetGamepadPowerInfo(pad,&percent);if(power==SDL_POWERSTATE_ERROR||power==SDL_POWERSTATE_UNKNOWN||percent<0)return std::wstring(L"n/d");if(power==SDL_POWERSTATE_NO_BATTERY)return std::wstring(L"sem bateria");return std::to_wstring(percent)+L"%";};std::wstring out;unsigned player=1;for(const auto& item:sources){if(player>4)break;SDL_Gamepad* pad=nullptr;bool temporary=false;if(source&&item.id==sourceId)pad=source;else for(const auto& extra:extraPlayers)if(extra.id==item.id){pad=extra.pad;break;}if(!pad&&initialized){pad=api.SDL_OpenGamepad(item.id);temporary=pad!=nullptr;}if(!out.empty())out+=L", ";out+=L"P"+std::to_wstring(player++)+L" "+value(pad);if(temporary)api.SDL_CloseGamepad(pad);}return L"Bateria: "+out;}
std::vector<ControllerSource> bridgeSources(){return sources;}
int bridgeSelfTest(std::string& log) {
 int failed=0;auto check=[&](bool good,const char* name){log+=(good?"PASS ":"FAIL ");log+=name;log+='\n';if(!good)++failed;};
 check(startSDL(),"load packaged SDL3 and initialize gamepad-only subsystem");if(!initialized)return failed;
 SDL_VirtualJoystickDesc description{};description.version=sizeof(description);description.type=SDL_JOYSTICK_TYPE_GAMEPAD;description.naxes=6;description.nbuttons=16;description.name="FeatherPad CI input";
 description.axis_mask=(1u<<SDL_GAMEPAD_AXIS_COUNT)-1;description.button_mask=(1u<<16)-1;
 const auto id=api.SDL_AttachVirtualJoystick(&description);check(id!=0,"attach isolated SDL test gamepad (no Windows driver)");
 SDL_Gamepad* test=id?api.SDL_OpenGamepad(id):nullptr;check(test!=nullptr,"open standardized SDL test gamepad");
 if(test){auto joystick=api.SDL_GetGamepadJoystick(test);api.SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,-32768);api.SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768);api.SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,true);api.SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,16000);api.SDL_UpdateGamepads();const auto state=readState(test);check((state.buttons&PadSouth)&&state.lx==16000,"read standardized buttons and analog axis through SDL");api.SDL_CloseGamepad(test);}
 if(id)api.SDL_DetachVirtualJoystick(id);
 std::vector<SDL_JoystickID> multiIds;std::vector<SDL_Gamepad*> multiPads;
 for(int player=0;player<4;++player){const auto multiId=api.SDL_AttachVirtualJoystick(&description);if(!multiId)break;multiIds.push_back(multiId);auto* pad=api.SDL_OpenGamepad(multiId);if(pad)multiPads.push_back(pad);}
 api.SDL_UpdateGamepads();bool independent=multiPads.size()==4;
 for(size_t player=0;player<multiPads.size();++player){const auto joystick=api.SDL_GetGamepadJoystick(multiPads[player]);const auto axis=static_cast<Sint16>((player+1)*5000);api.SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,axis);api.SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,player==2);}
 api.SDL_UpdateGamepads();for(size_t player=0;player<multiPads.size();++player){const auto state=readState(multiPads[player]);independent=independent&&state.lx==static_cast<int>((player+1)*5000)&&((state.buttons&PadSouth)!=0)==(player==2);}
 check(independent,"four independent physical input streams keep player buttons and axes separate");
 for(auto* pad:multiPads)api.SDL_CloseGamepad(pad);
 for(auto multiId:multiIds)api.SDL_DetachVirtualJoystick(multiId);
 bridgeShutdown();return failed;
}
