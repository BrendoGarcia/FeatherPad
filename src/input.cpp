#include "app.h"

Device* deviceFor(HANDLE handle) {
 auto found=devices.find(handle);if(found!=devices.end())return &found->second;
 UINT n=0;
 if(GetRawInputDeviceInfoW(handle,RIDI_PREPARSEDDATA,nullptr,&n)==static_cast<UINT>(-1)||!n||n>1024*1024)return nullptr;
 Device d;d.descriptor.resize(n);
 if(GetRawInputDeviceInfoW(handle,RIDI_PREPARSEDDATA,d.descriptor.data(),&n)==static_cast<UINT>(-1))return nullptr;
 auto pp=reinterpret_cast<PHIDP_PREPARSED_DATA>(d.descriptor.data());HIDP_CAPS caps{};
 if(HidP_GetCaps(pp,&caps)!=HIDP_STATUS_SUCCESS||caps.UsagePage!=1||(caps.Usage!=4&&caps.Usage!=5))return nullptr;
 USHORT count=caps.NumberInputValueCaps;d.values.resize(count);
 if(count&&HidP_GetValueCaps(HidP_Input,d.values.data(),&count,pp)!=HIDP_STATUS_SUCCESS)d.values.clear();else d.values.resize(count);
 UINT pathSize=0;GetRawInputDeviceInfoW(handle,RIDI_DEVICENAME,nullptr,&pathSize);
 if(controllerCompatibility&&pathSize&&pathSize<32768){std::wstring path(pathSize,L'\0');if(GetRawInputDeviceInfoW(handle,RIDI_DEVICENAME,&path[0],&pathSize)!=static_cast<UINT>(-1)&&isVirtualBusPath(path))return nullptr;}
 auto entry=devices.emplace(handle,std::move(d));
 status(L"Controle conectado. Sua biblioteca está pronta.");return &entry.first->second;
}
namespace {
 LONG signedValue(ULONG value,const HIDP_VALUE_CAPS& cap) {
  if(cap.LogicalMin<0&&cap.BitSize>0&&cap.BitSize<32&&(value&(1UL<<(cap.BitSize-1))))return static_cast<LONG>(value|(~0UL<<cap.BitSize));
  return static_cast<LONG>(value);
 }
 void report(HANDLE handle,char* bytes,ULONG length) {
  Device* d=deviceFor(handle);if(!d)return;
  auto pp=reinterpret_cast<PHIDP_PREPARSED_DATA>(d->descriptor.data());USAGE buttons[128]{};ULONG count=128;
  if(HidP_GetUsages(HidP_Input,9,0,buttons,&count,pp,bytes,length)!=HIDP_STATUS_SUCCESS)return;
  std::set<USAGE> now(buttons,buttons+count),previous=d->previous;d->previous=now;
  controllerButtons(handle,now,previous);
  const HWND foreground=GetForegroundWindow();
  if(captureWin||!IsWindowVisible(win)||IsIconic(win)||(foreground!=win&&foreground!=panel)) {d->direction=0;return;}
  if(panel&&page==Page::Diagnostic){d->direction=0;return;}
  int hatDirection=0,xDirection=0,yDirection=0;
  for(const auto& v:d->values) {
   if(v.UsagePage!=1)continue;
   const USAGE first=v.IsRange?v.Range.UsageMin:v.NotRange.Usage,last=v.IsRange?v.Range.UsageMax:first;
   for(USAGE usage:{static_cast<USAGE>(0x39),static_cast<USAGE>(0x30),static_cast<USAGE>(0x31)}) {
    if(usage<first||usage>last)continue;
    ULONG value=0;
    if(HidP_GetUsageValue(HidP_Input,1,v.LinkCollection,usage,&value,pp,bytes,length)!=HIDP_STATUS_SUCCESS)continue;
    const LONG val=signedValue(value,v);
    if(usage==0x39) {
     const LONG hat=val-v.LogicalMin;
     if(val>=v.LogicalMin&&val<=v.LogicalMax&&hat>=0&&hat<8){const int directions[]={-2,1,1,1,2,-1,-1,-1};hatDirection=directions[hat];}
    } else if(v.LogicalMax>v.LogicalMin) {
     const double normalized=(static_cast<double>(val)-v.LogicalMin)/(static_cast<double>(v.LogicalMax)-v.LogicalMin);
     const int dir=normalized<0.25?-1:normalized>0.75?1:0;
     if(usage==0x30)xDirection=dir;else yDirection=dir*2;
    }
   }
  }
  const int direction=hatDirection?hatDirection:(xDirection?xDirection:yDirection);
  const auto tick=GetTickCount64();
  if(direction&&(direction!=d->direction||tick>=d->repeat)){
   controllerDirection((direction==1||direction==-1)?direction:0,(direction==2||direction==-2)?direction/2:0);d->repeat=tick+(direction!=d->direction?400:150);
  }
  d->direction=direction;
 }
}
void rawInput(LPARAM l) {
 // SDL owns standardized launcher navigation while compatibility is enabled.
 // Never process its physical device twice or consume generated virtual pads.
 if(bridgeOwnsUI())return;
 UINT n=0;
 if(GetRawInputData(reinterpret_cast<HRAWINPUT>(l),RID_INPUT,nullptr,&n,sizeof(RAWINPUTHEADER))==static_cast<UINT>(-1)||n<sizeof(RAWINPUTHEADER)||n>1024*1024)return;
 std::vector<BYTE> buffer(n);
 if(GetRawInputData(reinterpret_cast<HRAWINPUT>(l),RID_INPUT,buffer.data(),&n,sizeof(RAWINPUTHEADER))!=n)return;
 auto r=reinterpret_cast<RAWINPUT*>(buffer.data());if(r->header.dwType!=RIM_TYPEHID)return;
 const auto& hid=r->data.hid;const size_t offset=static_cast<size_t>(hid.bRawData-buffer.data());
 if(!hid.dwSizeHid||offset>n||static_cast<size_t>(hid.dwCount)>(n-offset)/hid.dwSizeHid)return;
 for(DWORD i=0;i<hid.dwCount;++i)report(r->header.hDevice,reinterpret_cast<char*>(r->data.hid.bRawData)+static_cast<size_t>(i)*hid.dwSizeHid,hid.dwSizeHid);
}
void deviceChange(WPARAM w,LPARAM l) {
 HANDLE h=reinterpret_cast<HANDLE>(l);
 if(w==GIDC_REMOVAL) {
  devices.erase(h);captureArmed.erase(h);status(devices.empty()?L"Controle desconectado. Você também pode usar teclado e mouse.":L"Um controle foi desconectado.");
  if(captureWin&&!captureCandidate){captureNotice=L"O controle foi desconectado. Reconecte-o, solte os botões e pressione o botão desejado.";InvalidateRect(captureWin,nullptr,FALSE);}
 } else if(w==GIDC_ARRIVAL&&deviceFor(h)) {if(autoOpen&&!panel&&!captureWin)show();}
}
bool registerControllers() {
 RAWINPUTDEVICE input[2]={{1,4,RIDEV_INPUTSINK|RIDEV_DEVNOTIFY,win},{1,5,RIDEV_INPUTSINK|RIDEV_DEVNOTIFY,win}};
 const bool registered=RegisterRawInputDevices(input,2,sizeof(RAWINPUTDEVICE))!=FALSE;
 UINT n=0;GetRawInputDeviceList(nullptr,&n,sizeof(RAWINPUTDEVICELIST));
 if(n&&n<4096) {
  std::vector<RAWINPUTDEVICELIST> list(n);const UINT got=GetRawInputDeviceList(list.data(),&n,sizeof(RAWINPUTDEVICELIST));
  if(got!=static_cast<UINT>(-1))for(UINT i=0;i<got;++i)if(list[i].dwType==RIM_TYPEHID)deviceFor(list[i].hDevice);
 }
 return registered;
}
