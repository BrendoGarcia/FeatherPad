#pragma once
constexpr UINT_PTR LIBRARY_TIMER=602;
extern std::wstring collectionFilter;
std::vector<std::wstring> splitCollections(const std::wstring& text);
bool inCollection(const std::wstring& categories,const std::wstring& filter);
std::wstring newLibraryId();
void recordLaunch(size_t index,HANDLE process);
void libraryTick();
void finishSessions();
std::wstring systemMetrics();
std::wstring gameStatistics(size_t index);
