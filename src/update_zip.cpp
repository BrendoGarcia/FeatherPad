#include "update_common.h"
#include <zlib.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <algorithm>
namespace updates {
namespace {
using Bytes=std::vector<unsigned char>;
unsigned u16(const Bytes& b,size_t p){if(p+2>b.size())throw std::runtime_error("ZIP truncado.");return b[p]|(static_cast<unsigned>(b[p+1])<<8);}
std::uint32_t u32(const Bytes& b,size_t p){return u16(b,p)|(static_cast<std::uint32_t>(u16(b,p+2))<<16);}
struct Entry{std::string name;unsigned flags=0,method=0;std::uint32_t crc=0,compressed=0,size=0,offset=0;};
bool safeName(const std::string& n){if(n.empty()||n.size()>512||n.front()=='/'||n.find_first_of(":\\")!=std::string::npos||n.find('\0')!=std::string::npos)return false;size_t start=0;while(start<n.size()){auto end=n.find('/',start);auto part=n.substr(start,end==std::string::npos?end:end-start);if(part.empty()||part=="."||part=="..")return false;if(end==std::string::npos)break;start=end+1;}return true;}
}
std::vector<File> extractUpdateZip(const std::wstring& zip,const std::wstring& stage){
 std::ifstream input(std::filesystem::path(zip),std::ios::binary|std::ios::ate);if(!input)throw std::runtime_error("ZIP indisponível.");auto length=input.tellg();if(length<22||length>64*1024*1024)throw std::runtime_error("Tamanho de ZIP inválido (máximo 64 MB).");
 Bytes b(static_cast<size_t>(length));input.seekg(0);input.read(reinterpret_cast<char*>(b.data()),static_cast<std::streamsize>(b.size()));if(!input)throw std::runtime_error("ZIP truncado.");
 size_t end=b.size()-22;bool found=false;for(;;){if(u32(b,end)==0x06054b50&&end+22+u16(b,end+20)==b.size()){found=true;break;}if(end==0||b.size()-end>65557)break;--end;}if(!found)throw std::runtime_error("Diretório ZIP inválido.");
 unsigned count=u16(b,end+10);std::uint32_t central=u32(b,end+16),centralSize=u32(b,end+12);
 if(u16(b,end+4)||u16(b,end+6)||count!=u16(b,end+8)||count==0||count>256||centralSize>4*1024*1024||static_cast<std::uint64_t>(central)+centralSize!=end)throw std::runtime_error("ZIP64, multidisco ou diretório ZIP não suportado.");
 std::vector<Entry> entries;std::set<std::string> names;size_t p=central;std::string prefix;bool hasExe=false;
 for(unsigned i=0;i<count;++i){if(p+46>end||u32(b,p)!=0x02014b50)throw std::runtime_error("Entrada ZIP inválida.");Entry e;e.flags=u16(b,p+8);e.method=u16(b,p+10);e.crc=u32(b,p+16);e.compressed=u32(b,p+20);e.size=u32(b,p+24);e.offset=u32(b,p+42);unsigned nameLength=u16(b,p+28),extra=u16(b,p+30),comment=u16(b,p+32);size_t next=p+46+nameLength+extra+comment;if(next>end)throw std::runtime_error("ZIP truncado.");e.name.assign(reinterpret_cast<const char*>(b.data()+p+46),nameLength);p=next;
  if(!safeName(e.name)||(e.flags&1)||((u32(b,p-(46+nameLength+extra+comment)+38)>>16)&0170000)==0120000)throw std::runtime_error("ZIP com caminho inseguro, criptografia ou link simbólico.");
  std::string lower=e.name;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return static_cast<char>(c>='A'&&c<='Z'?c+32:c);});if(!names.insert(lower).second)throw std::runtime_error("ZIP com nomes duplicados.");
  if(e.name=="FeatherPad.exe"||(e.name.size()>15&&e.name.substr(e.name.size()-15)=="/FeatherPad.exe")){
   if(hasExe)throw std::runtime_error("ZIP com mais de um launcher.");
   hasExe=true;prefix=e.name.substr(0,e.name.size()-14);if(std::count(prefix.begin(),prefix.end(),'/')>1)throw std::runtime_error("Estrutura de pacote ZIP não reconhecida.");
  }
  entries.push_back(e);
 }
 if(p!=end||!hasExe)throw std::runtime_error("ZIP sem FeatherPad.exe válido.");
 std::vector<File> files;std::uint64_t total=0;
 const wchar_t* expected[]={L"FeatherPad.exe",L"Updater.exe",L"SDL3.dll",L"LICENSE",L"LEIA-ME.md",L"SDL3.txt",L"ViGEmClient.txt",L"zlib.txt"};
 for(const auto* name:expected){File f;f.name=name;f.path=allowedPath(name);const auto match=prefix+utf8(f.path);std::string normalized=match;std::replace(normalized.begin(),normalized.end(),'\\','/');auto it=std::find_if(entries.begin(),entries.end(),[&](const Entry& e){return e.name==normalized;});
  if(it==entries.end()){if(f.name==L"zlib.txt")continue;throw std::runtime_error("ZIP incompleto: "+utf8(f.path));}const auto& e=*it;total+=e.size;
  if(e.size==0||e.size>128*1024*1024||total>256*1024*1024||e.compressed>64*1024*1024||(e.method!=0&&e.method!=8)||static_cast<std::uint64_t>(e.offset)+30>central)throw std::runtime_error("ZIP excedeu limites ou usa compressão não suportada.");
  if(u32(b,e.offset)!=0x04034b50||u16(b,e.offset+8)!=e.method||u16(b,e.offset+6)!=e.flags)throw std::runtime_error("Cabeçalho ZIP divergente.");
  const size_t start=static_cast<size_t>(e.offset)+30+u16(b,e.offset+26)+u16(b,e.offset+28);if(start>central||static_cast<std::uint64_t>(start)+e.compressed>central)throw std::runtime_error("Dados ZIP fora do arquivo.");
  if(u16(b,e.offset+26)!=e.name.size()||std::string(reinterpret_cast<const char*>(b.data()+e.offset+30),e.name.size())!=e.name)throw std::runtime_error("Nome ZIP divergente.");
  std::ofstream out(std::filesystem::path(stage+L"\\"+f.name),std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Não foi possível extrair atualização.");
  uLong crc=crc32(0L,Z_NULL,0);std::uint64_t produced=0;
  if(e.method==0){if(e.compressed!=e.size)throw std::runtime_error("Tamanho ZIP divergente.");out.write(reinterpret_cast<const char*>(b.data()+start),e.size);crc=crc32(crc,b.data()+start,e.size);produced=e.size;}
  else {z_stream stream{};stream.next_in=b.data()+start;stream.avail_in=e.compressed;if(inflateInit2(&stream,-MAX_WBITS)!=Z_OK)throw std::runtime_error("Falha ao iniciar descompressão.");unsigned char buffer[65536];int status=Z_OK;bool valid=true;
   do{stream.next_out=buffer;stream.avail_out=sizeof(buffer);status=inflate(&stream,Z_NO_FLUSH);unsigned n=sizeof(buffer)-stream.avail_out;produced+=n;if(produced>e.size||(status!=Z_OK&&status!=Z_STREAM_END)){valid=false;break;}out.write(reinterpret_cast<const char*>(buffer),n);crc=crc32(crc,buffer,n);if(status==Z_OK&&n==0&&stream.avail_in==0){valid=false;break;}}while(status!=Z_STREAM_END);
   valid=valid&&status==Z_STREAM_END&&stream.avail_in==0;inflateEnd(&stream);if(!valid)throw std::runtime_error("Dados ZIP corrompidos.");
  }
  out.flush();if(!out||produced!=e.size||crc!=e.crc)throw std::runtime_error("CRC ou tamanho ZIP inválido.");out.close();f.size=e.size;f.hash=hashFile(stage+L"\\"+f.name);if(f.hash.empty())throw std::runtime_error("Falha ao verificar arquivo extraído.");files.push_back(f);
 }
 return files;
}
}
