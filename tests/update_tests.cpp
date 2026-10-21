#include "update_common.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <shellapi.h>
#include <sstream>
#include <algorithm>
#include "fixtures/zip_fixture.h"
int runTests(){
 // Run the real helper against a disposable installation, never the user's launcher.
 wchar_t self[32768]{};GetModuleFileNameW(nullptr,self,32768);
 std::filesystem::path selfPath(self);
 int argumentCount=0;LPWSTR* arguments=CommandLineToArgvW(GetCommandLineW(),&argumentCount);
 if(arguments&&argumentCount==3&&std::wstring(arguments[1])==L"--verify-package"){
  auto stage=std::filesystem::temp_directory_path()/(L"FeatherPad-package-test-"+std::to_wstring(GetCurrentProcessId()));std::filesystem::create_directories(stage);
  int result=0;try{auto files=updates::extractUpdateZip(arguments[2],stage.wstring());if(files.size()<7||updates::isOlderExecutable((stage/L"FeatherPad.exe").wstring(),(selfPath.parent_path()/L"FeatherPad.exe").wstring()))throw std::runtime_error("Pacote inválido.");std::cout<<"PASS real release ZIP extraction and executable version\n";}catch(const std::exception& e){std::cerr<<e.what();result=1;}
  LocalFree(arguments);std::filesystem::remove_all(stage);return result;
 }
 if(arguments)LocalFree(arguments);
 if(selfPath.filename()==L"FeatherPad.exe"){
  int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
  if(argc==3&&std::wstring(argv[1])==L"--fixture-parent"){
   std::wstring stage=argv[2],install=selfPath.parent_path().wstring();
   HANDLE parent=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,TRUE,GetCurrentProcessId());
   SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};HANDLE ready=CreateEventW(&attributes,TRUE,FALSE,nullptr);
   std::wstring exe=stage+L"\\runner.exe",cmd=L"\""+exe+L"\" "+std::to_wstring(reinterpret_cast<UINT_PTR>(parent))+L" \""+install+L"\" "+std::to_wstring(reinterpret_cast<UINT_PTR>(ready));
   STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
   bool ok=CreateProcessW(exe.c_str(),&cmd[0],nullptr,nullptr,TRUE,0,nullptr,stage.c_str(),&si,&pi)!=FALSE;
   if(ok){HANDLE signals[]={ready,pi.hProcess};ok=WaitForMultipleObjects(2,signals,FALSE,10000)==WAIT_OBJECT_0;CloseHandle(pi.hThread);CloseHandle(pi.hProcess);}if(ready)CloseHandle(ready);if(parent)CloseHandle(parent);LocalFree(argv);return ok?0:1;
  }
  if(argv)LocalFree(argv);
  std::ofstream(selfPath.parent_path()/L"restarted.txt")<<"restarted";return 0;
 }

 int failed=0;auto check=[&](bool ok,const char* text){std::cout<<(ok?"PASS ":"FAIL ")<<text<<'\n';if(!ok)++failed;};
 using namespace updates;
 check(newerVersion("v2.10.0","2.9.0"),"numeric semantic version comparison");
 for(auto v:{"v2.2.0","v2.1.9","v2.3.0-beta","v2.3.0.1","v2.3","garbage"})check(!newerVersion(v,"2.2.0"),"reject old, same or non stable version");
 check(parseJson("{\"tag_name\":\"v2.3.0\",\"assets\":[],\"text\":\"a\\\"b\\u00e7\"}").at("text").string()=="a\"b\xc3\xa7","JSON escapes and Unicode");
 for(auto bad:{"{","{\"x\":1,\"x\":2}","{} trailing","[1,]"}){bool rejected=false;try{parseJson(bad);}catch(...){rejected=true;}check(rejected,"reject malformed or duplicate JSON");}
 check(allowedPath(L"../FeatherPad.exe").empty()&&allowedPath(L"library.ini").empty()&&allowedPath(L"SDL3.txt")==L"licenses\\SDL3.txt","fixed update allowlist protects user data");

 auto releaseJson=[](const std::string& host,bool complete){
  std::string s="{\"draft\":false,\"prerelease\":false,\"assets\":[";
  const char* names[]={"FeatherPad.exe","Updater.exe","SDL3.dll","LICENSE","LEIA-ME.md","SDL3.txt","ViGEmClient.txt"};
  for(int i=0;i<(complete?7:6);++i){if(i)s+=",";s+="{\"name\":\""+std::string(names[i])+"\",\"size\":3,\"digest\":\"sha256:"+std::string(64,'a')+"\",\"browser_download_url\":\""+host+"/v2.3.0/"+names[i]+"\"}";}return s+"]}";
 };
 check(releaseFiles(parseJson(releaseJson("https://github.com/BrendoGarcia/FeatherPad/releases/download",true))).size()==7,"complete official release accepted");
 for(const auto& payload:{releaseJson("https://evil.example/releases/download",true),releaseJson("https://github.com/BrendoGarcia/FeatherPad/releases/download",false)}){
  bool rejected=false;try{releaseFiles(parseJson(payload));}catch(...){rejected=true;}check(rejected,"foreign host or incomplete release rejected");
 }
 const auto root=std::filesystem::temp_directory_path()/(L"FeatherPad-update-test-"+std::to_wstring(GetCurrentProcessId()));std::filesystem::remove_all(root);std::filesystem::create_directories(root/L"stage");std::filesystem::create_directory(root/L"install");
 auto write=[](const std::filesystem::path& p,const char* s){std::ofstream o(p,std::ios::binary);o<<s;};
 write(root/L"stage"/L"LICENSE","abc");File f;f.name=L"LICENSE";f.path=L"LICENSE";f.size=3;f.hash="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
 check(verifyFile((root/L"stage"/L"LICENSE").wstring(),f),"SHA-256 known vector");write(root/L"install"/L"LICENSE","old");writePlan((root/L"stage").wstring(),{f});
 auto plan=readPlan((root/L"stage").wstring());check(plan.size()==1&&plan[0].hash==f.hash,"update plan round trip");
 check(applyFiles((root/L"stage").wstring(),(root/L"install").wstring(),plan)&&verifyFile((root/L"install"/L"LICENSE").wstring(),f),"apply verified file and keep rollback");
 check(std::filesystem::exists(root/L"stage"/L"rollback"/L"LICENSE"),"previous file retained");
 std::filesystem::create_directory(root/L"broken");write(root/L"broken"/L"LICENSE","bad");check(!applyFiles((root/L"broken").wstring(),(root/L"install").wstring(),plan)&&verifyFile((root/L"install"/L"LICENSE").wstring(),f),"reject altered payload before touching installation");
 std::filesystem::create_directory(root/L"locked");write(root/L"locked"/L"LICENSE","abc");write(root/L"locked"/L"LEIA-ME.md","abc");write(root/L"install"/L"LICENSE","old");write(root/L"install"/L"LEIA-ME.md","old");
 File second=f;second.name=second.path=L"LEIA-ME.md";
 HANDLE lock=CreateFileW((root/L"install"/L"LEIA-ME.md").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
 check(lock!=INVALID_HANDLE_VALUE,"create locked destination fixture");
 bool applied=applyFiles((root/L"locked").wstring(),(root/L"install").wstring(),{f,second});check(!applied&&hashFile((root/L"install"/L"LICENSE").wstring())!=f.hash,"rollback on locked destination");if(lock!=INVALID_HANDLE_VALUE)CloseHandle(lock);


 auto fixture=root/L"fixture.zip";{std::ofstream out(fixture,std::ios::binary);out.write(reinterpret_cast<const char*>(zipFixture),sizeof(zipFixture));}
 std::filesystem::create_directory(root/L"extracted");auto extracted=extractUpdateZip(fixture.wstring(),(root/L"extracted").wstring());
 check(extracted.size()==7&&extracted[0].hash==f.hash,"deflated ZIP extraction checks CRC and hashes");
 std::filesystem::create_directory(root/L"same-install");std::filesystem::create_directory(root/L"same-install"/L"licenses");
 for(const auto& item:extracted)std::filesystem::copy_file(root/L"extracted"/item.name,root/L"same-install"/item.path);
 check(changedFiles(extracted,(root/L"same-install").wstring()).empty(),"already updated means identical contents, not tag ordering");
 write(root/L"same-install"/L"FeatherPad.exe","different");check(changedFiles(extracted,(root/L"same-install").wstring()).size()==1,"same internal version with changed executable remains detectable");
 auto zipMetadata=parseJson("{\"draft\":false,\"prerelease\":false,\"tag_name\":\"1.0.1\",\"assets\":[{\"name\":\"FeatherPad-Windows-x64.zip\",\"digest\":\"sha256:"+std::string(64,'a')+"\",\"size\":123,\"browser_download_url\":\"https://github.com/BrendoGarcia/FeatherPad/releases/download/1.0.1/FeatherPad-Windows-x64.zip\"}]}");
 check(releaseDistribution(zipMetadata).archive,"older tag-only ZIP release is still recognized while the internal version has advanced beyond the tag");
 auto corrupt=std::vector<unsigned char>(std::begin(zipFixture),std::end(zipFixture));corrupt[44]^=255;
 {std::ofstream out(root/L"corrupt.zip",std::ios::binary);out.write(reinterpret_cast<const char*>(corrupt.data()),corrupt.size());}
 bool rejected=false;try{extractUpdateZip((root/L"corrupt.zip").wstring(),(root/L"extracted").wstring());}catch(...){rejected=true;}check(rejected,"corrupt ZIP payload is rejected before installation");

 auto unsafe=std::vector<unsigned char>(std::begin(zipFixture),std::end(zipFixture));
 const std::string original="FeatherPad.exe",traversal="../evilapp.exe";
 for(auto pos=std::search(unsafe.begin(),unsafe.end(),original.begin(),original.end());pos!=unsafe.end();pos=std::search(pos+original.size(),unsafe.end(),original.begin(),original.end()))std::copy(traversal.begin(),traversal.end(),pos);
 {std::ofstream out(root/L"unsafe.zip",std::ios::binary);out.write(reinterpret_cast<const char*>(unsafe.data()),unsafe.size());}
 rejected=false;try{extractUpdateZip((root/L"unsafe.zip").wstring(),(root/L"extracted").wstring());}catch(...){rejected=true;}check(rejected&&!std::filesystem::exists(root/L"evilapp.exe"),"ZIP traversal cannot write outside staging");
 // Real Updater process: inherited handle survives parent exit; restart uses original path.
 auto e2e=root/L"e2e",install=e2e/L"install",stage=e2e/L"stage";
 std::filesystem::create_directories(install);std::filesystem::create_directory(stage);
 std::filesystem::copy_file(selfPath,install/L"FeatherPad.exe");
 std::filesystem::copy_file(selfPath.parent_path()/L"Updater.exe",stage/L"runner.exe");
 write(install/L"LICENSE","old");write(stage/L"LICENSE","abc");writePlan(stage.wstring(),{f});
 std::wstring cmd=L"\""+(install/L"FeatherPad.exe").wstring()+L"\" --fixture-parent \""+stage.wstring()+L"\"";
 STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
 bool started=CreateProcessW((install/L"FeatherPad.exe").c_str(),&cmd[0],nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,install.c_str(),&si,&pi)!=FALSE;
 check(started,"start disposable launcher process");if(started){WaitForSingleObject(pi.hProcess,10000);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);}
 auto deadline=GetTickCount64()+10000;
 while(started&&!std::filesystem::exists(install/L"restarted.txt")&&GetTickCount64()<deadline)Sleep(50);
 check(std::filesystem::exists(install/L"restarted.txt")&&verifyFile((install/L"LICENSE").wstring(),f),"real helper waits, applies and restarts launcher");
 if(std::filesystem::exists(stage/L"result.txt")){std::ifstream in(stage/L"result.txt");std::cout<<in.rdbuf();}
 // The restarted fixture may be finishing its final instructions. Retry only fixture cleanup.
 std::error_code ec;
 for(int i=0;i<20;++i){std::filesystem::remove_all(root,ec);if(!ec)break;Sleep(50);}
 return failed?1:0;
}

int main(){std::cout<<std::unitbuf;try{return runTests();}catch(const std::exception& e){std::cerr<<"FAIL unexpected exception: "<<e.what()<<std::endl;return 1;}}
