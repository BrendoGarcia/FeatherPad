#pragma once
#include <cstdint>
#include <string>
struct MemorySample {
 bool valid=false,privateResidentValid=false;
 std::uint64_t residentBytes=0,privateResidentBytes=0,privateCommitBytes=0,peakResidentBytes=0;
 unsigned long processId=0;
 unsigned long long sampledAt=0;
};
enum class NetworkKind { Unknown, Offline, WiFi, Ethernet, Other };
struct NetworkSample {
 NetworkKind kind=NetworkKind::Unknown;
 bool connectivityKnown=false,internet=false;
 int signalPercent=-1;
 std::wstring adapter;
};
constexpr unsigned METRICS_READY=0x8000+30;
MemorySample readProcessMemory();
MemorySample currentMemory();
NetworkSample readNetwork();
NetworkSample currentNetwork();
void refreshSystemMetrics(bool force=false);
void shutdownSystemMetrics();
std::wstring formatMiB(std::uint64_t bytes);
std::wstring memoryHeadline();
std::wstring memoryDetails();
std::wstring networkHeadline(const NetworkSample& sample);
std::wstring networkDetails();
int wifiBars(int percent);
