#include "app.h"
#include <psapi.h>
#include <netlistmgr.h>
#include <cwctype>
#include <limits>
std::wstring collectionFilter;
namespace {
struct Session {std::wstring id;HANDLE process;ULONGLONG tick;};
std::vector<Session> sessions;
ULONGLONG lastSave=0;
std::wstring metrics=L"Medição pendente";
bool same(const std::wstring& a,const std::wstring& b){return CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_EQUAL;}
bool samePath(std::wstring a,std::wstring b){auto normalize=[](std::wstring& p){if(p.rfind(L"\\\\?\\",0)==0)p.erase(0,4);std::replace(p.begin(),p.end(),L'/',L'\\');while(p.size()>3&&p.back()==L'\\')p.pop_back();};normalize(a);normalize(b);return same(a,b);}
}
std::wstring newLibraryId(){GUID id{};if(FAILED(CoCreateGuid(&id)))return L"";wchar_t value[40]{};StringFromGUID2(id,value,40);return value;}
std::vector<std::wstring> splitCollections(const std::wstring& text){
 std::vector<std::wstring> out;size_t begin=0;
 while(begin<=text.size()&&out.size()<16){auto end=text.find(L';',begin);auto v=text.substr(begin,end==std::wstring::npos?end:end-begin);auto first=v.find_first_not_of(L" \t"),last=v.find_last_not_of(L" \t");if(first!=std::wstring::npos){v=v.substr(first,last-first+1);if(v.size()<=40&&std::none_of(out.begin(),out.end(),[&](const auto& x){return same(x,v);}))out.push_back(v);}if(end==std::wstring::npos)break;begin=end+1;}
 return out;
}
bool inCollection(const std::wstring& categories,const std::wstring& filter){if(filter.empty())return true;for(const auto& c:splitCollections(categories))if(same(c,filter))return true;return false;}
void recordLaunch(size_t index,HANDLE process){
 if(index>=games.size()){if(process)CloseHandle(process);return;}
 auto& g=games[index];if(g.id.empty())g.id=newLibraryId();if(g.launches<std::numeric_limits<unsigned long long>::max())++g.launches;
 FILETIME now{};GetSystemTimeAsFileTime(&now);ULARGE_INTEGER value{};value.LowPart=now.dwLowDateTime;value.HighPart=now.dwHighDateTime;g.lastLaunch=value.QuadPart;
 // Only count a direct executable whose returned process is that executable.
 wchar_t actual[32768]{},expected[32768]{};DWORD size=32768;
 bool direct=process&&g.path.size()>4&&same(g.path.substr(g.path.size()-4),L".exe")&&GetFullPathNameW(g.path.c_str(),32768,expected,nullptr)&&QueryFullProcessImageNameW(process,0,actual,&size)&&samePath(actual,expected);
 DWORD pid=process?GetProcessId(process):0;
 if(direct&&sessions.size()<32&&std::none_of(sessions.begin(),sessions.end(),[&](const auto& s){return GetProcessId(s.process)==pid;}))sessions.push_back({g.id,process,GetTickCount64()});else if(process)CloseHandle(process);
 saveDatabase();
}
void libraryTick(){
 bool dirty=false;const auto now=GetTickCount64();
 for(auto it=sessions.begin();it!=sessions.end();){
  const bool ended=WaitForSingleObject(it->process,0)!=WAIT_TIMEOUT;
  auto seconds=(now-it->tick)/1000;
  if(ended){FILETIME created{},exit{},kernel{},user{};if(GetProcessTimes(it->process,&created,&exit,&kernel,&user)){ULARGE_INTEGER end{};end.LowPart=exit.dwLowDateTime;end.HighPart=exit.dwHighDateTime;FILETIME current{};GetSystemTimeAsFileTime(&current);ULARGE_INTEGER wall{};wall.LowPart=current.dwLowDateTime;wall.HighPart=current.dwHighDateTime;if(wall.QuadPart>=end.QuadPart)seconds=seconds> (wall.QuadPart-end.QuadPart)/10000000?seconds-(wall.QuadPart-end.QuadPart)/10000000:0;}}
  for(auto& g:games)if(g.id==it->id){g.playedSeconds+=seconds;dirty|=seconds>0;break;}
  it->tick+=seconds*1000;
  if(ended){CloseHandle(it->process);it=sessions.erase(it);dirty=true;}else ++it;
 }
 if(dirty&&(now-lastSave>=30000||sessions.empty())){saveDatabase();lastSave=now;}
 static ULONGLONG lastMetrics=0;
 if(now-lastMetrics>=2000&&IsWindowVisible(win)&&!IsIconic(win)){
  PROCESS_MEMORY_COUNTERS_EX mem{};mem.cb=sizeof(mem);std::wstring ram=L"RAM: indisponível";
  if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&mem),sizeof(mem)))ram=L"RAM privada: "+std::to_wstring(mem.PrivateUsage/1048576)+L" MB | residente: "+std::to_wstring(mem.WorkingSetSize/1048576)+L" MB";
  std::wstring net=L"Internet: desconhecida";INetworkListManager* manager=nullptr;
  if(SUCCEEDED(CoCreateInstance(CLSID_NetworkListManager,nullptr,CLSCTX_ALL,IID_INetworkListManager,reinterpret_cast<void**>(&manager)))){NLM_CONNECTIVITY state{};if(SUCCEEDED(manager->GetConnectivity(&state)))net=(state&(NLM_CONNECTIVITY_IPV4_INTERNET|NLM_CONNECTIVITY_IPV6_INTERNET))?L"Internet: conectada":state?L"Internet: só rede local":L"Internet: desconectada";manager->Release();}
  metrics=ram+L"   ·   "+bridgeBatteryStatus()+L"   ·   "+bridgePlayerSummary()+L"   ·   "+net;lastMetrics=now;InvalidateRect(win,nullptr,FALSE);
 }
}
void finishSessions(){libraryTick();for(auto& s:sessions)CloseHandle(s.process);if(!sessions.empty())saveDatabase();sessions.clear();}
std::wstring systemMetrics(){return metrics;}
std::wstring gameStatistics(size_t index){if(index>=games.size())return L"";const auto& g=games[index];std::wstring last=L"Nunca";if(g.lastLaunch){ULARGE_INTEGER v{};v.QuadPart=g.lastLaunch;FILETIME ft{v.LowPart,v.HighPart},local{};SYSTEMTIME time{};if(FileTimeToLocalFileTime(&ft,&local)&&FileTimeToSystemTime(&local,&time)){wchar_t value[80]{};swprintf(value,80,L"%02u/%02u/%04u às %02u:%02u",time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute);last=value;}}
 return L"Último início: "+last+L"\r\n\r\nAberturas bem-sucedidas: "+std::to_wstring(g.launches)+L"\r\n\r\nTempo acompanhado: "+std::to_wstring(g.playedSeconds/3600)+L" h "+std::to_wstring(g.playedSeconds/60%60)+L" min\r\n\r\nO tempo só é contado para executáveis diretos confirmados, enquanto o launcher permanece aberto. URIs, atalhos e clientes externos podem não fornecer o processo do jogo.";
}
