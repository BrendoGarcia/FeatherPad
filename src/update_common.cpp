#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "update_common.h"
#include <bcrypt.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <array>
#include <algorithm>
#include <cctype>
#include <limits>
#include <set>
namespace updates {
namespace {
class Parser {
 const std::string& s;size_t p=0;
 void ws(){while(p<s.size()&&(s[p]==' '||s[p]=='\r'||s[p]=='\n'||s[p]=='\t'))++p;}
 char take(){if(p>=s.size())throw std::runtime_error("JSON truncado.");return s[p++];}
 unsigned hex4(){unsigned v=0;for(int i=0;i<4;++i){char c=take();v*=16;if(c>='0'&&c<='9')v+=c-'0';else if(c>='a'&&c<='f')v+=c-'a'+10;else if(c>='A'&&c<='F')v+=c-'A'+10;else throw std::runtime_error("Escape JSON inválido.");}return v;}
 std::string str(){if(take()!='"')throw std::runtime_error("JSON inválido.");std::string out;for(;;){unsigned char c=take();if(c=='"')return out;if(c<32)throw std::runtime_error("Texto JSON inválido.");if(c!='\\'){out+=static_cast<char>(c);continue;}char e=take();switch(e){case '"':case '\\':case '/':out+=e;break;case 'b':out+='\b';break;case 'f':out+='\f';break;case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;case 'u':{unsigned u=hex4();std::wstring w(1,static_cast<wchar_t>(u));if(u>=0xd800&&u<=0xdbff){if(take()!='\\'||take()!='u')throw std::runtime_error("Unicode inválido.");unsigned low=hex4();if(low<0xdc00||low>0xdfff)throw std::runtime_error("Unicode inválido.");w+=static_cast<wchar_t>(low);}else if(u>=0xdc00&&u<=0xdfff)throw std::runtime_error("Unicode inválido.");out+=utf8(w);break;}default:throw std::runtime_error("Escape inválido.");}}}
 Json val(int depth){if(depth>32)throw std::runtime_error("JSON muito profundo.");ws();if(p>=s.size())throw std::runtime_error("JSON vazio.");Json j;char c=s[p];if(c=='"'){j.kind='s';j.value=str();}
 else if(c=='{'||c=='['){++p;j.kind=c;ws();char end=c=='{'?'}':']';if(p<s.size()&&s[p]==end){++p;return j;}for(;;){ws();if(c=='{'){auto key=str();ws();if(take()!=':')throw std::runtime_error("JSON inválido.");if(!j.object.emplace(key,val(depth+1)).second)throw std::runtime_error("Chave JSON repetida.");}else j.array.push_back(val(depth+1));ws();char delim=take();if(delim==end)break;if(delim!=',')throw std::runtime_error("JSON inválido.");}}
 else {size_t start=p;while(p<s.size()&&s[p]!=','&&s[p]!=']'&&s[p]!='}'&&!std::isspace(static_cast<unsigned char>(s[p])))++p;j.value=s.substr(start,p-start);if(j.value=="true"||j.value=="false")j.kind='b';else if(j.value=="null")j.kind='n';else{j.kind='d';if(j.value.empty())throw std::runtime_error("Número inválido.");for(char n:j.value)if((n<'0'||n>'9')&&n!='-'&&n!='+'&&n!='.'&&n!='e'&&n!='E')throw std::runtime_error("JSON inválido.");}}
 return j;}
public:explicit Parser(const std::string& input):s(input){}Json parse(){auto j=val(0);ws();if(p!=s.size())throw std::runtime_error("Dados após JSON.");return j;}
};
bool digestValid(const std::string& s){return s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});}
bool versionParts(std::string s,std::array<unsigned,3>& v){if(!s.empty()&&s[0]=='v')s.erase(0,1);std::istringstream in(s);std::string part;for(int i=0;i<3;++i){if(!std::getline(in,part,'.')||part.empty()||part.size()>6)return false;if(!std::all_of(part.begin(),part.end(),[](char c){return c>='0'&&c<='9';}))return false;v[i]=static_cast<unsigned>(std::stoul(part));}return in.eof();}
bool ordinary(const std::wstring& p){DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&!(a&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT));}
bool safeParent(const std::wstring& p){DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_DIRECTORY)&&!(a&FILE_ATTRIBUTE_REPARSE_POINT);}
}
std::uint64_t Json::number()const{if(kind!='d'||value.empty()||value.size()>18||!std::all_of(value.begin(),value.end(),[](char c){return c>='0'&&c<='9';}))throw std::runtime_error("Tamanho inválido.");return std::stoull(value);}
Json parseJson(const std::string& s){if(s.size()>4*1024*1024)throw std::runtime_error("Metadados grandes demais.");return Parser(s).parse();}
bool newerVersion(const std::string& a,const std::string& b){std::array<unsigned,3> x{},y{};return versionParts(a,x)&&versionParts(b,y)&&x>y;}
std::wstring wide(const std::string& s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);if(!n&&!s.empty())throw std::runtime_error("UTF-8 inválido.");std::wstring out(n,L'\0');if(n)MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),&out[0],n);return out;}
std::string utf8(const std::wstring& s){int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0,nullptr,nullptr);if(!n&&!s.empty())throw std::runtime_error("Unicode inválido.");std::string out(n,'\0');if(n)WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),&out[0],n,nullptr,nullptr);return out;}
std::wstring executableDirectory(){wchar_t p[32768]{};GetModuleFileNameW(nullptr,p,32768);std::wstring s=p;return s.substr(0,s.find_last_of(L"\\/"));}
std::wstring allowedPath(const std::wstring& name){if(name==L"FeatherPad.exe"||name==L"Updater.exe"||name==L"SDL3.dll"||name==L"LICENSE"||name==L"LEIA-ME.md")return name;if(name==L"SDL3.txt"||name==L"ViGEmClient.txt")return L"licenses\\"+name;return L"";}
std::vector<File> releaseFiles(const Json& release){
 if(release.at("draft").value!="false"||release.at("prerelease").value!="false")throw std::runtime_error("A release não é estável.");
 const auto& assets=release.at("assets");if(assets.kind!='[')throw std::runtime_error("Lista de arquivos inválida.");
 std::vector<File> files;std::map<std::wstring,bool> seen;
 for(const auto& a:assets.array){File f;f.name=wide(a.at("name").string());f.path=allowedPath(f.name);if(f.path.empty())continue;
  if(seen[f.name])throw std::runtime_error("Arquivo duplicado na release.");
  seen[f.name]=true;
  f.url=a.at("browser_download_url").string();const std::string prefix="https://github.com/BrendoGarcia/FeatherPad/releases/download/";
  if(f.url.rfind(prefix,0)!=0)throw std::runtime_error("Origem de download inválida.");
  auto digest=a.at("digest").string();if(digest.rfind("sha256:",0)!=0||!digestValid(digest.substr(7)))throw std::runtime_error("Release sem SHA-256 confiável do GitHub.");f.hash=digest.substr(7);f.size=a.at("size").number();if(!f.size||f.size>128*1024*1024)throw std::runtime_error("Tamanho de arquivo fora do limite.");files.push_back(f);
 }
 if(files.size()!=7)throw std::runtime_error("Release sem os 7 arquivos de atualização. Use o workflow Release Windows.");
 return files;
}
std::string hashFile(const std::wstring& path){
 if(!ordinary(path))return "";
 std::ifstream input(std::filesystem::path(path),std::ios::binary);if(!input)return "";
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE h=nullptr;DWORD objectSize=0,read=0;std::string result;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return "";
 if(BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectSize),sizeof(objectSize),&read,0)<0){BCryptCloseAlgorithmProvider(alg,0);return "";}
 std::vector<unsigned char> object(objectSize);unsigned char digest[32]{};bool ok=BCryptCreateHash(alg,&h,object.data(),objectSize,nullptr,0,0)>=0;
 char buffer[65536];while(ok&&input){input.read(buffer,sizeof(buffer));if(input.gcount()>0)ok=BCryptHashData(h,reinterpret_cast<PUCHAR>(buffer),static_cast<ULONG>(input.gcount()),0)>=0;}
 ok=ok&&input.eof()&&BCryptFinishHash(h,digest,sizeof(digest),0)>=0;
 if(ok){const char* digits="0123456789abcdef";for(auto c:digest){result+=digits[c>>4];result+=digits[c&15];}}
 if(h)BCryptDestroyHash(h);
 BCryptCloseAlgorithmProvider(alg,0);return result;
}
bool verifyFile(const std::wstring& p,const File& f){WIN32_FILE_ATTRIBUTE_DATA a{};return GetFileAttributesExW(p.c_str(),GetFileExInfoStandard,&a)&&!a.nFileSizeHigh&&a.nFileSizeLow==f.size&&hashFile(p)==f.hash;}
void writePlan(const std::wstring& stage,const std::vector<File>& files){std::ofstream out(std::filesystem::path(stage+L"\\plan.txt"),std::ios::binary);if(!out)throw std::runtime_error("Não foi possível salvar o plano.");for(const auto& f:files)out<<utf8(f.name)<<' '<<f.hash<<' '<<f.size<<'\n';out.flush();if(!out)throw std::runtime_error("Falha ao salvar o plano.");}
std::vector<File> readPlan(const std::wstring& stage){std::ifstream in(std::filesystem::path(stage+L"\\plan.txt"));std::vector<File> files;std::string line;std::set<std::wstring> names;while(std::getline(in,line)){if(line.size()>200)throw std::runtime_error("Plano inválido.");std::istringstream row(line);File f;std::string name,extra;if(!(row>>name>>f.hash>>f.size)||(row>>extra))throw std::runtime_error("Plano inválido.");f.name=wide(name);f.path=allowedPath(f.name);if(f.path.empty()||!names.insert(f.name).second||!digestValid(f.hash)||!f.size||f.size>128*1024*1024||files.size()>=7)throw std::runtime_error("Plano inválido.");files.push_back(f);}if(files.empty())throw std::runtime_error("Plano vazio.");
 return files;}
bool applyFiles(const std::wstring& stage,const std::wstring& install,const std::vector<File>& files){
 if(!safeParent(stage)||!safeParent(install))return false;
 const auto backup=stage+L"\\rollback";if(!CreateDirectoryW(backup.c_str(),nullptr))return false;
 std::vector<File> changed;std::vector<bool> existed;
 // Validate every file and prepare rollback copies BEFORE touching the installation.
 for(const auto& f:files){if(allowedPath(f.name)!=f.path||!verifyFile(stage+L"\\"+f.name,f))return false;
  const auto dest=install+L"\\"+f.path;const bool has=GetFileAttributesW(dest.c_str())!=INVALID_FILE_ATTRIBUTES;
  if(f.path.rfind(L"licenses\\",0)==0){const auto folder=install+L"\\licenses";CreateDirectoryW(folder.c_str(),nullptr);if(!safeParent(folder))return false;}
  if(has&&(!ordinary(dest)||!CopyFileW(dest.c_str(),(backup+L"\\"+f.name).c_str(),TRUE)))return false;
  existed.push_back(has);
 }
 for(size_t i=0;i<files.size();++i){const auto& f=files[i];const auto dest=install+L"\\"+f.path;
  // Include the failing copy too: CopyFile may already have truncated its destination.
  changed.push_back(f);
  if(!CopyFileW((stage+L"\\"+f.name).c_str(),dest.c_str(),FALSE)||!verifyFile(dest,f)){
   bool restored=true;for(size_t j=0;j<changed.size();++j){const auto p=install+L"\\"+changed[j].path;if(existed[j]&&hashFile(p)==hashFile(backup+L"\\"+changed[j].name))continue;
    if(existed[j])restored=CopyFileW((backup+L"\\"+changed[j].name).c_str(),p.c_str(),FALSE)&&restored;else if(GetFileAttributesW(p.c_str())!=INVALID_FILE_ATTRIBUTES)restored=DeleteFileW(p.c_str())&&restored;}
   if(!restored)throw std::runtime_error("Restauração incompleta. Preserve a pasta rollback e restaure os arquivos manualmente.");
   return false;
  }
 }
 return true;
}
}
