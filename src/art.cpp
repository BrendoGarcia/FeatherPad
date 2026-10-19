#include "app.h"

namespace {
 std::map<std::wstring,Art> cache;
 constexpr size_t maxThumbnails=6;
 template<class T> struct Com {
  T* p=nullptr;
  ~Com(){if(p)p->Release();}
  T** address(){return &p;}
  T* operator->()const{return p;}
 };
 bool openImage(const std::wstring& path,Com<IWICBitmapDecoder>& decoder,Com<IWICBitmapFrameDecode>& frame,UINT& w,UINT& h) {
  if(!imaging||path.empty())return false;
  WIN32_FILE_ATTRIBUTE_DATA attr{};
  if(!GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&attr)||attr.nFileSizeHigh||attr.nFileSizeLow>8*1024*1024)return false;
  if(FAILED(imaging->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,decoder.address())))return false;
  return SUCCEEDED(decoder->GetFrame(0,frame.address())) && SUCCEEDED(frame->GetSize(&w,&h)) && w>0 && h>0 && w<=2048 && h<=2048;
 }
 Art decode(const std::wstring& path) {
  Art result;Com<IWICBitmapDecoder> decoder;Com<IWICBitmapFrameDecode> frame;UINT w=0,h=0;
  if(!openImage(path,decoder,frame,w,h))return result;
  // Scale before materializing pixels; do not retain a full-resolution bitmap.
  const double ratio=std::min(300.0/w,420.0/h);
  const UINT outW=std::max(1u,static_cast<UINT>(w*std::min(1.0,ratio)));
  const UINT outH=std::max(1u,static_cast<UINT>(h*std::min(1.0,ratio)));
  Com<IWICBitmapScaler> scaler;Com<IWICFormatConverter> converter;
  if(FAILED(imaging->CreateBitmapScaler(scaler.address())) || FAILED(scaler->Initialize(frame.p,outW,outH,WICBitmapInterpolationModeLinear)))return result;
  if(FAILED(imaging->CreateFormatConverter(converter.address())) || FAILED(converter->Initialize(scaler.p,GUID_WICPixelFormat32bppBGR,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return result;
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=static_cast<LONG>(outW);
  info.bmiHeader.biHeight=-static_cast<LONG>(outH);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
  if(!bitmap)return result;
  if(FAILED(converter->CopyPixels(nullptr,outW*4,outW*outH*4,static_cast<BYTE*>(pixels)))) {DeleteObject(bitmap);return result;}
  result.bitmap=bitmap;result.width=static_cast<int>(outW);result.height=static_cast<int>(outH);return result;
 }
}
bool validateArt(const std::wstring& path) {
 // Test the actual decoder as well as metadata, including truncated/corrupt files.
 auto art=decode(path);const bool valid=art.bitmap!=nullptr;if(art.bitmap)DeleteObject(art.bitmap);return valid;
}
const Art* artFor(const std::wstring& path) {
 if(path.empty())return nullptr;
 auto found=cache.find(path);
 if(found!=cache.end()){found->second.used=GetTickCount64();return found->second.bitmap?&found->second:nullptr;}
 if(cache.size()>=maxThumbnails) {
  auto oldest=std::min_element(cache.begin(),cache.end(),[](const auto& a,const auto& b){return a.second.used<b.second.used;});
  if(oldest->second.bitmap)DeleteObject(oldest->second.bitmap);
  cache.erase(oldest);
 }
 Art art=decode(path);art.used=GetTickCount64();
 auto entry=cache.emplace(path,art);return entry.first->second.bitmap?&entry.first->second:nullptr;
}
void clearArt() {
 for(auto& entry:cache)if(entry.second.bitmap)DeleteObject(entry.second.bitmap);
 cache.clear();
}
void drawArt(HDC dc,const Art& art,RECT target) {
 if(!art.bitmap)return;
 const int tw=target.right-target.left,th=target.bottom-target.top;
 const double ratio=std::max(static_cast<double>(tw)/art.width,static_cast<double>(th)/art.height);
 const int sw=std::max(1,static_cast<int>(tw/ratio)),sh=std::max(1,static_cast<int>(th/ratio));
 HDC image=CreateCompatibleDC(dc);HGDIOBJ old=SelectObject(image,art.bitmap);
 const int mode=SetStretchBltMode(dc,HALFTONE);POINT origin{};SetBrushOrgEx(dc,0,0,&origin);
 StretchBlt(dc,target.left,target.top,tw,th,image,(art.width-sw)/2,(art.height-sh)/2,sw,sh,SRCCOPY);
 SetBrushOrgEx(dc,origin.x,origin.y,nullptr);SetStretchBltMode(dc,mode);SelectObject(image,old);DeleteDC(image);
}

bool saveThumbnail(const std::wstring& source,const std::wstring& destination) {
 auto art=decode(source);if(!art.bitmap)return false;
 Com<IWICBitmap> bitmap;Com<IWICStream> stream;Com<IWICBitmapEncoder> encoder;Com<IWICBitmapFrameEncode> frame;
 bool good=SUCCEEDED(imaging->CreateBitmapFromHBITMAP(art.bitmap,nullptr,WICBitmapIgnoreAlpha,bitmap.address()));
 good=good&&SUCCEEDED(imaging->CreateStream(stream.address()))&&SUCCEEDED(stream->InitializeFromFilename(destination.c_str(),GENERIC_WRITE));
 good=good&&SUCCEEDED(imaging->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.address()))&&SUCCEEDED(encoder->Initialize(stream.p,WICBitmapEncoderNoCache));
 good=good&&SUCCEEDED(encoder->CreateNewFrame(frame.address(),nullptr))&&SUCCEEDED(frame->Initialize(nullptr));
 WICPixelFormatGUID format=GUID_WICPixelFormat24bppBGR;
 good=good&&SUCCEEDED(frame->SetSize(static_cast<UINT>(art.width),static_cast<UINT>(art.height)))&&SUCCEEDED(frame->SetPixelFormat(&format));
 good=good&&SUCCEEDED(frame->WriteSource(bitmap.p,nullptr))&&SUCCEEDED(frame->Commit())&&SUCCEEDED(encoder->Commit());
 DeleteObject(art.bitmap);return good;
}
