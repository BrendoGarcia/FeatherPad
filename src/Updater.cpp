#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include "update_common.h"

int WINAPI wWinMain(HINSTANCE,HINSTANCE,LPWSTR,int){
 int count=0;LPWSTR* args=CommandLineToArgvW(GetCommandLineW(),&count);if(!args)return 1;
 if(count!=3){LocalFree(args);MessageBoxW(nullptr,L"Este componente é iniciado pelo FeatherPad após verificar uma atualização.",L"FeatherPad Updater",MB_OK|MB_ICONINFORMATION);return 1;}
 std::wstring install=args[2],stage=updates::executableDirectory();HANDLE parent=nullptr;
 try{size_t used=0;auto number=std::stoull(args[1],&used);if(used!=wcslen(args[1])||!number)throw std::runtime_error("Processo inválido.");parent=reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(number));}catch(...){LocalFree(args);return 1;}LocalFree(args);
 auto restart=[&](){std::wstring command=L"\""+install+L"\\FeatherPad.exe\"";STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};bool ok=CreateProcessW((install+L"\\FeatherPad.exe").c_str(),&command[0],nullptr,nullptr,FALSE,0,nullptr,install.c_str(),&si,&pi)!=FALSE;if(ok){CloseHandle(pi.hThread);CloseHandle(pi.hProcess);}return ok;};
 bool waited=false;HANDLE guard=nullptr;int code=1;std::wstring message;
 try{
  wchar_t image[32768]{};DWORD size=32768;const auto expected=install+L"\\FeatherPad.exe";
  if(!QueryFullProcessImageNameW(parent,0,image,&size)||CompareStringOrdinal(image,-1,expected.c_str(),-1,TRUE)!=CSTR_EQUAL)throw std::runtime_error("O processo de origem não corresponde à instalação.");
  if(WaitForSingleObject(parent,30000)!=WAIT_OBJECT_0)throw std::runtime_error("O launcher não encerrou a tempo. Nenhum arquivo foi alterado.");
  waited=true;CloseHandle(parent);parent=nullptr;
  guard=CreateMutexW(nullptr,TRUE,L"Local\\FeatherPad.Singleton.v1");
  if(!guard||GetLastError()==ERROR_ALREADY_EXISTS)throw std::runtime_error("Outra instância foi aberta durante a atualização. Feche-a e tente novamente.");
  auto files=updates::readPlan(stage);
  if(!updates::applyFiles(stage,install,files))throw std::runtime_error("Não foi possível aplicar a atualização. Os arquivos substituídos foram restaurados. Confira permissões, antivírus e espaço em disco.");
  code=0;message=L"Atualização aplicada. Cópia anterior preservada em: "+stage+L"\\rollback";
 }catch(const std::exception& e){message=updates::wide(e.what());}
 if(parent)CloseHandle(parent);
 if(guard)CloseHandle(guard);
 std::ofstream log(std::filesystem::path(stage+L"\\result.txt"));log<<updates::utf8(message)<<'\n';log.close();
 if(code)MessageBoxW(nullptr,(message+L"\n\nDiagnóstico e recuperação: "+stage).c_str(),L"FeatherPad — atualização",MB_ICONERROR);
 if(waited&&!restart()){MessageBoxW(nullptr,L"Não foi possível reiniciar. Abra FeatherPad.exe manualmente. A pasta de recuperação foi preservada.",L"FeatherPad",MB_ICONERROR);return 1;}
 return code;
}
