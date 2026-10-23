#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mutex>
#include <cstdint>
#include <thread>
// One stable, owned channel per player. Driver I/O never calls SDL or the UI.
class RumbleChannel {
public:
 struct Output {std::uint16_t low=0,high=0;bool send=false;};
 ~RumbleChannel();
 bool start(void* driver,void* target,bool playStation);
 void stop();
 void publish(std::uint8_t large,std::uint8_t small);
 Output consume(bool allow,ULONGLONG now);
private:
 std::mutex mailbox;std::uint64_t command=0;bool enabled=false;
 HANDLE cancel=nullptr;std::thread worker;
 std::uint64_t consumed=0;ULONGLONG refresh=0;bool running=false;
};
