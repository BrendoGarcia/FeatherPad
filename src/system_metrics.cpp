#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include "app.h"
#include <psapi.h>
#include <iphlpapi.h>
#include <wlanapi.h>
#include <netlistmgr.h>
#include <thread>
#include <mutex>
#include <atomic>

namespace {
 MemorySample memory;
 NetworkSample network;
 std::mutex networkMutex;
 std::thread networkWorker;
 std::atomic<bool> pending{false},shuttingDown{false};
 ULONGLONG lastMemory=0,lastNetwork=0;
}
MemorySample readProcessMemory(){
 MemorySample out;out.processId=GetCurrentProcessId();out.sampledAt=GetTickCount64();
 // Count private resident pages directly. No generated estimate or scaling factor.
 std::vector<ULONG_PTR> buffer(8192);
 for(int attempt=0;attempt<7;++attempt){
  if(QueryWorkingSet(GetCurrentProcess(),buffer.data(),static_cast<DWORD>(buffer.size()*sizeof(ULONG_PTR)))){
   const auto* pages=reinterpret_cast<const PSAPI_WORKING_SET_INFORMATION*>(buffer.data());
   const size_t capacity=(buffer.size()*sizeof(ULONG_PTR)-offsetof(PSAPI_WORKING_SET_INFORMATION,WorkingSetInfo))/sizeof(PSAPI_WORKING_SET_BLOCK);
   if(pages->NumberOfEntries<=capacity){SYSTEM_INFO info{};GetSystemInfo(&info);std::uint64_t privatePages=0;for(ULONG_PTR i=0;i<pages->NumberOfEntries;++i)if(!pages->WorkingSetInfo[i].Shared)++privatePages;out.privateResidentBytes=privatePages*info.dwPageSize;out.privateResidentValid=true;}
   break;
  }
  if(GetLastError()!=ERROR_BAD_LENGTH)break;
  buffer.resize(buffer.size()*2);
 }
 PROCESS_MEMORY_COUNTERS_EX counters{};counters.cb=sizeof(counters);
 if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters))){
  out.valid=true;out.residentBytes=counters.WorkingSetSize;out.privateCommitBytes=counters.PrivateUsage;out.peakResidentBytes=counters.PeakWorkingSetSize;
 }
 return out;
}
std::wstring formatMiB(std::uint64_t bytes){
 const auto tenths=(bytes*10+524288)/1048576;
 return std::to_wstring(tenths/10)+L","+std::to_wstring(tenths%10)+L" MiB";
}
MemorySample currentMemory(){return memory;}
NetworkSample currentNetwork(){std::lock_guard<std::mutex> lock(networkMutex);return network;}
int wifiBars(int percent){if(percent<0)return -1;if(percent==0)return 0;return std::min(4,(std::min(100,percent)+24)/25);}
NetworkSample readNetwork(){
 NetworkSample out;
 const HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 INetworkListManager* manager=nullptr;
 if(SUCCEEDED(initialized)&&SUCCEEDED(CoCreateInstance(CLSID_NetworkListManager,nullptr,CLSCTX_INPROC_SERVER,IID_INetworkListManager,reinterpret_cast<void**>(&manager)))){
  NLM_CONNECTIVITY state{};if(SUCCEEDED(manager->GetConnectivity(&state))){out.connectivityKnown=true;out.internet=(state&(NLM_CONNECTIVITY_IPV4_INTERNET|NLM_CONNECTIVITY_IPV6_INTERNET))!=0;}
  manager->Release();
 }
 ULONG size=16384;std::vector<BYTE> adapters(size);
 ULONG result=GetAdaptersAddresses(AF_UNSPEC,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER,nullptr,reinterpret_cast<IP_ADAPTER_ADDRESSES*>(adapters.data()),&size);
 if(result==ERROR_BUFFER_OVERFLOW&&size<=1024*1024){adapters.resize(size);result=GetAdaptersAddresses(AF_UNSPEC,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER,nullptr,reinterpret_cast<IP_ADAPTER_ADDRESSES*>(adapters.data()),&size);}
 DWORD routeIndex=0;
 // Local routing-table lookup only: no connection or packet is sent.
 GetBestInterface(0x01010101,&routeIndex);
 DWORD selectedIndex=0;
 if(result==NO_ERROR){
  IP_ADAPTER_ADDRESSES* selected=nullptr;
  for(auto* item=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(adapters.data());item;item=item->Next){
   if(item->OperStatus!=IfOperStatusUp||item->IfType==IF_TYPE_SOFTWARE_LOOPBACK)continue;
   if(!selected)selected=item;
   if(item->IfIndex==routeIndex){selected=item;break;}
  }
  if(selected){
   selectedIndex=selected->IfIndex;out.adapter=selected->FriendlyName?selected->FriendlyName:L"";
   out.kind=selected->IfType==IF_TYPE_IEEE80211?NetworkKind::WiFi:selected->IfType==IF_TYPE_ETHERNET_CSMACD?NetworkKind::Ethernet:NetworkKind::Other;
  }else out.kind=NetworkKind::Offline;
 }else if(out.connectivityKnown&&!out.internet)out.kind=NetworkKind::Offline;
 if(out.kind==NetworkKind::WiFi){
  HANDLE client=nullptr;DWORD version=0;
  if(WlanOpenHandle(2,nullptr,&version,&client)==ERROR_SUCCESS){
   PWLAN_INTERFACE_INFO_LIST interfaces=nullptr;
   if(WlanEnumInterfaces(client,nullptr,&interfaces)==ERROR_SUCCESS){
    for(DWORD i=0;i<interfaces->dwNumberOfItems;++i){
     auto& item=interfaces->InterfaceInfo[i];NET_LUID luid{};NET_IFINDEX index=0;
     if(ConvertInterfaceGuidToLuid(&item.InterfaceGuid,&luid)!=NO_ERROR||ConvertInterfaceLuidToIndex(&luid,&index)!=NO_ERROR||index!=selectedIndex)continue;
     DWORD bytes=0;PVOID data=nullptr;WLAN_OPCODE_VALUE_TYPE type{};
     if(WlanQueryInterface(client,&item.InterfaceGuid,wlan_intf_opcode_current_connection,nullptr,&bytes,&data,&type)==ERROR_SUCCESS){
      if(bytes>=sizeof(WLAN_CONNECTION_ATTRIBUTES)){const auto* connection=static_cast<const WLAN_CONNECTION_ATTRIBUTES*>(data);if(connection->isState==wlan_interface_state_connected)out.signalPercent=static_cast<int>(std::min(100ul,connection->wlanAssociationAttributes.wlanSignalQuality));}
      WlanFreeMemory(data);
     }
    }
    WlanFreeMemory(interfaces);
   }
   WlanCloseHandle(client,nullptr);
  }
 }
 if(SUCCEEDED(initialized))CoUninitialize();
 return out;
}
void sampleNetworkNow(){
 NetworkSample sample;try{sample=readNetwork();}catch(...){}
 std::lock_guard<std::mutex> lock(networkMutex);network=std::move(sample);
}
void refreshSystemMetrics(bool force){
 const auto now=GetTickCount64();
 if(force||!memory.valid||now-lastMemory>=1000){memory=readProcessMemory();lastMemory=now;}
 if(networkWorker.joinable()&&!pending.load())networkWorker.join();
 if(!selfTesting&&!shuttingDown.load()&&!pending.load()&&(force||now-lastNetwork>=3000)){
  lastNetwork=now;pending=true;
  try {networkWorker=std::thread([]{
   sampleNetworkNow();
   pending=false;if(win)PostMessageW(win,METRICS_READY,0,0);
  });}catch(...){pending=false;}
 }
 if(win)InvalidateRect(win,nullptr,FALSE);
}
void shutdownSystemMetrics(){shuttingDown=true;if(networkWorker.joinable())networkWorker.join();}
std::wstring memoryHeadline(){
 if(!memory.valid)return L"RAM: leitura indisponível";
 return std::wstring(memory.privateResidentValid?L"RAM privada: ":L"RAM residente: ")+formatMiB(memory.privateResidentValid?memory.privateResidentBytes:memory.residentBytes);
}
std::wstring memoryDetails(){
 if(!memory.valid)return L"O Windows não disponibilizou os contadores deste processo.";
 return L"FeatherPad.exe · PID "+std::to_wstring(memory.processId)+L"\r\n\r\nRAM privada residente: "+(memory.privateResidentValid?formatMiB(memory.privateResidentBytes)+L" ("+std::to_wstring(memory.privateResidentBytes)+L" bytes)":L"indisponível")+L"\r\nRAM residente total: "+formatMiB(memory.residentBytes)+L" ("+std::to_wstring(memory.residentBytes)+L" bytes)\r\nMemória privada comprometida: "+formatMiB(memory.privateCommitBytes)+L"\r\nPico de RAM residente: "+formatMiB(memory.peakResidentBytes)+L"\r\n\r\nConsulta real ao Windows, atualizada a cada segundo. RAM privada conta páginas residentes exclusivas do app (QueryWorkingSet). RAM total e memória comprometida vêm de GetProcessMemoryInfo. O jogo e os drivers não entram nessa medição. Valores podem variar com capas e janelas abertas.";
}
std::wstring networkHeadline(const NetworkSample& sample){
 switch(sample.kind){
 case NetworkKind::WiFi:return sample.signalPercent<0?L"Wi-Fi · sinal indisponível":L"Wi-Fi · "+std::to_wstring(sample.signalPercent)+L"%"+(sample.connectivityKnown&&!sample.internet?L" · sem internet":L"");
 case NetworkKind::Ethernet:return sample.connectivityKnown&&!sample.internet?L"Ethernet · sem internet":L"Ethernet · cabo conectado";
 case NetworkKind::Other:return sample.connectivityKnown&&!sample.internet?L"Rede · sem internet":L"Rede conectada";
 case NetworkKind::Offline:return L"Sem conexão";
 default:return L"Consultando rede…";
 }
}
std::wstring networkDetails(){
 const auto s=currentNetwork();
 return networkHeadline(s)+L"\r\n"+s.adapter+L"\r\n"+(s.connectivityKnown?(s.internet?L"Acesso à internet detectado pelo Windows.":L"O Windows não detectou acesso à internet."):L"Estado de acesso à internet indisponível.")+L"\r\n\r\nAs barras mostram a qualidade do enlace Wi-Fi reportada pelo driver (WlanQueryInterface), não a velocidade da internet. Ethernet usa um ícone de cabo. Quando o Windows não fornece o sinal, nenhuma intensidade é inventada.";
}
