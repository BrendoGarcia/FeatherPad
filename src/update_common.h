#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdint>
namespace updates {
constexpr const char* version=FEATHERPAD_VERSION;
struct Json {
 std::string value;std::map<std::string,Json> object;std::vector<Json> array;char kind='n';
 const Json& at(const std::string& key)const {auto i=object.find(key);if(i==object.end())throw std::runtime_error("Metadados incompletos.");return i->second;}
 std::string string()const{if(kind!='s')throw std::runtime_error("Texto esperado.");return value;}
 std::uint64_t number()const;
};
Json parseJson(const std::string& input);
bool newerVersion(const std::string& candidate,const std::string& current);
struct File {std::wstring name,path;std::string hash,url;std::uint64_t size=0;};
std::vector<File> releaseFiles(const Json& release);
struct Distribution { std::vector<File> files;File zip;bool archive=false; };
Distribution releaseDistribution(const Json& release);
std::vector<File> extractUpdateZip(const std::wstring& zip,const std::wstring& stage);
std::vector<File> changedFiles(const std::vector<File>& files,const std::wstring& install);
bool isOlderExecutable(const std::wstring& candidate,const std::wstring& installed);
std::wstring wide(const std::string& s);
std::string utf8(const std::wstring& s);
std::wstring executableDirectory();
std::string hashFile(const std::wstring& path);
std::wstring allowedPath(const std::wstring& name);
void writePlan(const std::wstring& stage,const std::vector<File>& files);
std::vector<File> readPlan(const std::wstring& stage);
bool verifyFile(const std::wstring& path,const File& file);
bool applyFiles(const std::wstring& stage,const std::wstring& install,const std::vector<File>& files);
}
