#include "rumble.h"
#include <climits>
#include <winioctl.h>
#include <ViGEm/km/BusShared.h>
#include <ViGEm/Client.h>
// Layouts come from the exact statically linked SDK revision pinned in CMake.
// Its legacy notification API detaches threads; this reader cancels and joins
// before the associated target/client is freed, including disconnect/reconfigure.
#include "Internal.h"

RumbleChannel::~RumbleChannel(){stop();}
void RumbleChannel::publish(std::uint8_t large,std::uint8_t small){
 std::lock_guard<std::mutex> guard(mailbox);if(!enabled)return;
 command=((command+0x100000000ULL)&0xffffffff00000000ULL)|large|(static_cast<std::uint64_t>(small)<<8);
}
RumbleChannel::Output RumbleChannel::consume(bool allow,ULONGLONG now){
 std::lock_guard<std::mutex> guard(mailbox);Output out;
 if(!allow){enabled=false;command=0;consumed=0;out.send=running;running=false;refresh=0;return out;}
 enabled=true;const auto latest=command;
 const bool changed=latest!=consumed;
 if(changed){consumed=latest;running=(latest&0xffff)!=0;}
 if(changed||(running&&now>=refresh)){out.low=static_cast<std::uint16_t>((latest&255)*257);out.high=static_cast<std::uint16_t>(((latest>>8)&255)*257);out.send=true;refresh=now+120;}
 return out;
}
bool RumbleChannel::start(void* driver,void* device,bool sony){
 stop();if(!driver||!device)return false;
 auto client=static_cast<PVIGEM_CLIENT>(driver);auto target=static_cast<PVIGEM_TARGET>(device);const auto bus=client->hBusDevice;const ULONG serial=target->SerialNo;
 if(bus==INVALID_HANDLE_VALUE||serial==0)return false;
 cancel=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!cancel)return false;
 try {worker=std::thread([this,bus,serial,sony]{
  HANDLE done=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!done)return;
  HANDLE waits[]={cancel,done};
  while(WaitForSingleObject(cancel,0)==WAIT_TIMEOUT){
   OVERLAPPED overlapped{};overlapped.hEvent=done;ResetEvent(done);DWORD bytes=0;
   XUSB_REQUEST_NOTIFICATION xbox;XUSB_REQUEST_NOTIFICATION_INIT(&xbox,serial);
   DS4_REQUEST_NOTIFICATION ds4;DS4_REQUEST_NOTIFICATION_INIT(&ds4,serial);
   void* report=sony?static_cast<void*>(&ds4):static_cast<void*>(&xbox);const DWORD size=sony?ds4.Size:xbox.Size;
   BOOL good=DeviceIoControl(bus,sony?IOCTL_DS4_REQUEST_NOTIFICATION:IOCTL_XUSB_REQUEST_NOTIFICATION,report,size,report,size,&bytes,&overlapped);
   if(!good&&GetLastError()==ERROR_IO_PENDING){
    const auto result=WaitForMultipleObjects(2,waits,FALSE,INFINITE);
    if(result!=WAIT_OBJECT_0+1){CancelIoEx(bus,&overlapped);GetOverlappedResult(bus,&overlapped,&bytes,TRUE);break;}
    good=GetOverlappedResult(bus,&overlapped,&bytes,FALSE);
   }
   if(!good)break; // Disconnect/error must not spin.
   if(sony)publish(ds4.Report.LargeMotor,ds4.Report.SmallMotor);else publish(xbox.LargeMotor,xbox.SmallMotor);
  }
  CloseHandle(done);
 });}catch(...){CloseHandle(cancel);cancel=nullptr;return false;}
 return true;
}
void RumbleChannel::stop(){
 {std::lock_guard<std::mutex> guard(mailbox);enabled=false;}
 if(cancel)SetEvent(cancel);
 if(worker.joinable())worker.join();
 if(cancel)CloseHandle(cancel);
 cancel=nullptr;std::lock_guard<std::mutex> guard(mailbox);command=0;consumed=0;refresh=0;running=false;
}
