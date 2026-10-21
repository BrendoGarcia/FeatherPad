#include "app.h"
#include "update_common.h"
#include <winhttp.h>
#include <thread>
#include <atomic>
#include <filesystem>
#include <fstream>

namespace {
 std::thread worker;std::atomic<bool> busy{false},stopping{false};
 struct Result {std::wstring error,stage,tag;bool available=false,downloaded=false,archive=false;updates::File zip;std::vector<updates::File> files;} result;
 struct Http {HINTERNET h=nullptr;~Http(){if(h)WinHttpCloseHandle(h);}};
 bool allowedHost(const std::wstring& h){return h==L"api.github.com"||h==L"github.com"||h==L"release-assets.githubusercontent.com"||h==L"objects.githubusercontent.com";}
 void get(const std::wstring& url,std::string* text,const std::wstring& file,std::uint64_t limit){
  const auto start=GetTickCount64();URL_COMPONENTS u{};u.dwStructSize=sizeof(u);u.dwHostNameLength=u.dwUrlPathLength=u.dwExtraInfoLength=static_cast<DWORD>(-1);
  if(!WinHttpCrackUrl(url.c_str(),0,0,&u)||u.nScheme!=INTERNET_SCHEME_HTTPS||u.nPort!=443)throw std::runtime_error("URL HTTPS inválida.");
  const std::wstring host(u.lpszHostName,u.dwHostNameLength);if(!allowedHost(host))throw std::runtime_error("Servidor de atualização inválido.");
  std::wstring path(u.lpszUrlPath,u.dwUrlPathLength);if(u.dwExtraInfoLength)path.append(u.lpszExtraInfo,u.dwExtraInfoLength);
  Http session;session.h=WinHttpOpen(L"FeatherPad/2.2",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!session.h)throw std::runtime_error("Não foi possível iniciar HTTPS.");
  WinHttpSetTimeouts(session.h,10000,10000,15000,15000);DWORD tls=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;WinHttpSetOption(session.h,WINHTTP_OPTION_SECURE_PROTOCOLS,&tls,sizeof(tls));
  Http connection;connection.h=WinHttpConnect(session.h,host.c_str(),443,0);if(!connection.h)throw std::runtime_error("Falha de conexão.");
  Http request;request.h=WinHttpOpenRequest(connection.h,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);if(!request.h)throw std::runtime_error("Falha na consulta.");
  DWORD redirect=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request.h,WINHTTP_OPTION_REDIRECT_POLICY,&redirect,sizeof(redirect));
  const wchar_t* headers=L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
  if(!WinHttpSendRequest(request.h,headers,static_cast<DWORD>(-1),WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request.h,nullptr))throw std::runtime_error("Falha HTTPS. Confira sua conexão e tente novamente.");
  DWORD code=0,size=sizeof(code);WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&code,&size,WINHTTP_NO_HEADER_INDEX);
  if(code==301||code==302||code==303||code==307||code==308){static thread_local int depth=0;if(depth>=4)throw std::runtime_error("Redirecionamentos demais.");DWORD n=0;WinHttpQueryHeaders(request.h,WINHTTP_QUERY_LOCATION,WINHTTP_HEADER_NAME_BY_INDEX,nullptr,&n,WINHTTP_NO_HEADER_INDEX);if(!n||n>32768)throw std::runtime_error("Redirecionamento inválido.");std::wstring next(n/sizeof(wchar_t),L'\0');if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_LOCATION,WINHTTP_HEADER_NAME_BY_INDEX,&next[0],&n,WINHTTP_NO_HEADER_INDEX))throw std::runtime_error("Redirecionamento inválido.");next.resize(wcslen(next.c_str()));++depth;try{get(next,text,file,limit);}catch(...){--depth;throw;}--depth;return;}
  if(code==404)throw std::runtime_error("Nenhuma release pública compatível disponível. Publique uma versão pelo workflow Release Windows.");
  if(code==403||code==429)throw std::runtime_error("Limite do GitHub ou acesso negado. Tente novamente mais tarde.");
  if(code!=200)throw std::runtime_error("Resposta inesperada do servidor: "+std::to_string(code));
  std::ofstream output;if(!file.empty()){output.open(std::filesystem::path(file),std::ios::binary|std::ios::trunc);if(!output)throw std::runtime_error("Não foi possível criar o download.");}
  char buffer[65536];std::uint64_t total=0;
  for(;;){if(stopping||GetTickCount64()-start>180000)throw std::runtime_error("Download cancelado ou tempo limite atingido.");DWORD n=0;if(!WinHttpReadData(request.h,buffer,sizeof(buffer),&n))throw std::runtime_error("Download interrompido.");if(!n)break;total+=n;if(total>limit)throw std::runtime_error("Download excedeu o tamanho permitido.");if(text)text->append(buffer,n);else {output.write(buffer,n);if(!output)throw std::runtime_error("Falha ao gravar download.");}}
  if(output.is_open()){output.flush();if(!output)throw std::runtime_error("Falha ao concluir download.");}
 }
 void start(bool download){
  if(busy.exchange(true))return;
  if(worker.joinable())worker.join();
  worker=std::thread([download]{try{
   if(!download){
    result={};std::string body;get(L"https://api.github.com/repos/BrendoGarcia/FeatherPad/releases/latest",&body,L"",4*1024*1024);
    auto release=updates::parseJson(body);result.tag=updates::wide(release.at("tag_name").string());
    auto distribution=updates::releaseDistribution(release);result.archive=distribution.archive;result.zip=distribution.zip;
    result.files=distribution.files;result.available=result.archive||!updates::changedFiles(result.files,updates::executableDirectory()).empty();
   }
   else {
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Não foi possível preparar atualização.");wchar_t guid[40]{};StringFromGUID2(id,guid,40);
    auto root=dataFolder+L"\\updates";CreateDirectoryW(root.c_str(),nullptr);result.stage=root+L"\\"+guid;if(!CreateDirectoryW(result.stage.c_str(),nullptr))throw std::runtime_error("Não foi possível criar a pasta temporária.");
    std::vector<updates::File> changed;auto install=updates::executableDirectory();
    if(result.archive){
     const auto package=result.stage+L"\\package.zip";get(updates::wide(result.zip.url),nullptr,package,result.zip.size);
     if(!updates::verifyFile(package,result.zip))throw std::runtime_error("SHA-256 do ZIP inválido. A instalação foi preservada.");
     result.files=updates::extractUpdateZip(package,result.stage);
    }
    for(const auto& f:result.files){
     if(stopping)throw std::runtime_error("Atualização cancelada.");
     if(updates::verifyFile(install+L"\\"+f.path,f))continue;
     if(!result.archive){get(updates::wide(f.url),nullptr,result.stage+L"\\"+f.name,f.size);if(!updates::verifyFile(result.stage+L"\\"+f.name,f))throw std::runtime_error("SHA-256 ou tamanho inválido. A instalação foi preservada.");}
     changed.push_back(f);
    }
    if(changed.empty()){result.available=false;result.downloaded=false;}
    else {
    auto exe=std::find_if(changed.begin(),changed.end(),[](const updates::File& f){return f.name==L"FeatherPad.exe";});
    if(exe!=changed.end()&&updates::isOlderExecutable(result.stage+L"\\FeatherPad.exe",install+L"\\FeatherPad.exe"))throw std::runtime_error("A release encontrada contém um executável anterior ao instalado. Não será feito downgrade. A tag pública e a versão interna são mostradas separadamente.");
    updates::writePlan(result.stage,changed);
    // Execute a copy of the CURRENT trusted helper, never the freshly downloaded helper.
    if(!CopyFileW((install+L"\\Updater.exe").c_str(),(result.stage+L"\\runner.exe").c_str(),TRUE))throw std::runtime_error("Updater.exe ausente. Extraia o pacote completo desta versão uma vez.");
    result.downloaded=true;
    }
   }
  }catch(const std::exception& e){result.error=updates::wide(e.what());}catch(...){result.error=L"Falha inesperada ao atualizar.";}
  if(!stopping)PostMessageW(win,UPDATE_READY,0,0);
  });
 }
}
void checkForUpdates(){if(busy){MessageBoxW(panel?panel:win,L"Uma consulta ou download já está em andamento.",L"Atualizações",MB_OK|MB_ICONINFORMATION);return;}status(L"Consultando a última release estável no GitHub…");start(false);}
void handleUpdateResult(){
 if(worker.joinable())worker.join();
 busy=false;HWND owner=keyboardWindow?keyboardWindow:(captureWin?captureWin:(panel?panel:win));
 if(!result.error.empty()){status(result.error);MessageBoxW(owner,result.error.c_str(),L"Atualização não aplicada",MB_ICONWARNING);return;}
 if(!result.downloaded){if(!result.available){status(L"Arquivos conferidos com a release "+result.tag+L". Nenhuma alteração necessária.");MessageBoxW(owner,(L"Release "+result.tag+L": todos os arquivos verificados já correspondem à instalação.").c_str(),L"FeatherPad",MB_OK|MB_ICONINFORMATION);return;}
  const auto question=result.archive?L"A release oferece um ZIP. Baixar e verificar seu conteúdo para comparar com os arquivos instalados?":L"Há arquivos diferentes na release. Baixar e verificar a atualização?";
  if(MessageBoxW(owner,(L"Release publicada: "+result.tag+L"\nVersão interna instalada: "+updates::wide(updates::version)+L"\n\n"+question+L"\n\nBiblioteca, capas e configurações serão preservadas.").c_str(),L"Verificar conteúdo da release",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;
  status(L"Baixando e verificando os arquivos alterados…");start(true);return;
 }
 if(MessageBoxW(owner,L"Download verificado. Fechar o launcher, aplicar a atualização e reiniciar agora?\n\nNão atualize durante uma partida: a conversão do controle será interrompida.",L"Instalar atualização",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return;
 const auto install=updates::executableDirectory();
 HANDLE parent=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,GetCurrentProcessId());
 if(!parent||!SetHandleInformation(parent,HANDLE_FLAG_INHERIT,HANDLE_FLAG_INHERIT)){if(parent)CloseHandle(parent);MessageBoxW(owner,L"Não foi possível preparar o processo para atualizar.",L"FeatherPad",MB_ICONERROR);return;}
 SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
 HANDLE ready=CreateEventW(&attributes,TRUE,FALSE,nullptr);if(!ready){CloseHandle(parent);return;}
 std::wstring command=L"\""+result.stage+L"\\runner.exe\" "+std::to_wstring(reinterpret_cast<UINT_PTR>(parent))+L" \""+install+L"\" "+std::to_wstring(reinterpret_cast<UINT_PTR>(ready));
 STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
 if(!CreateProcessW((result.stage+L"\\runner.exe").c_str(),&command[0],nullptr,nullptr,TRUE,0,nullptr,result.stage.c_str(),&si,&pi)){CloseHandle(parent);CloseHandle(ready);MessageBoxW(owner,L"Não foi possível iniciar o atualizador. O launcher continua aberto.",L"FeatherPad",MB_ICONERROR);return;}
 HANDLE signals[]={ready,pi.hProcess};DWORD state=WaitForMultipleObjects(2,signals,FALSE,10000);
 CloseHandle(parent);CloseHandle(ready);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
 if(state!=WAIT_OBJECT_0){MessageBoxW(owner,L"O atualizador não confirmou a preparação. O launcher permanece aberto e a instalação não foi substituída.",L"FeatherPad",MB_ICONERROR);return;}
 if(keyboardWindow)closeVirtualKeyboard(false);
 closePanel();DestroyWindow(win);
}
void shutdownUpdates(){stopping=true;if(worker.joinable())worker.join();}
