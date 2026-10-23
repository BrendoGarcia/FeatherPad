#pragma once
#include <windows.h>
#include <string>
constexpr int COVER_IMPORT=810;
struct CoverCrop {double x=.5,y=.5,zoom=1;};
extern HWND coverPreviewWindow;
RECT coverCropRect(int width,int height,const CoverCrop& crop);
bool saveCroppedThumbnail(const std::wstring& source,const std::wstring& destination,const CoverCrop& crop);
void openCoverPreview(const std::wstring& path);
void closeCoverPreview(bool apply,bool activate=true);
void coverPreviewDirection(int dx,int dy);
void coverPreviewConfirm();
bool coverPreviewRoute(MSG& message);
void applyCoverPreview(const std::wstring& path,const CoverCrop& crop);
