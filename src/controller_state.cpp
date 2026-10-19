#include "controller_state.h"
#include <algorithm>
#include <cstdlib>

ControllerMode resolveControllerMode(ControllerMode game,ControllerMode global) {
 if(game!=ControllerMode::Auto)return game;
 return global==ControllerMode::Auto?ControllerMode::Xbox:global;
}
namespace {
 std::int16_t invertY(std::int16_t value){return value==-32768?32767:static_cast<std::int16_t>(-value);}
 std::uint8_t sonyAxis(std::int16_t value){return static_cast<std::uint8_t>((static_cast<int>(value)+32768)/256);}
}
XboxState toXbox(const PadState& s) {
 XboxState out;out.lt=s.lt;out.rt=s.rt;out.lx=s.lx;out.ly=invertY(s.ly);out.rx=s.rx;out.ry=invertY(s.ry);
 const std::uint32_t from[]={PadUp,PadDown,PadLeft,PadRight,PadStart,PadBack,PadLStick,PadRStick,PadLShoulder,PadRShoulder,PadSouth,PadEast,PadWest,PadNorth};
 const std::uint16_t to[]={0x0001,0x0002,0x0004,0x0008,0x0010,0x0020,0x0040,0x0080,0x0100,0x0200,0x1000,0x2000,0x4000,0x8000};
 for(unsigned i=0;i<14;++i)if(s.buttons&from[i])out.buttons|=to[i];
 // Guide / PS is reserved for opening the launcher, not forwarded to overlays.
 return out;
}
PlayStationState toPlayStation(const PadState& s) {
 PlayStationState out;out.lx=sonyAxis(s.lx);out.ly=sonyAxis(s.ly);out.rx=sonyAxis(s.rx);out.ry=sonyAxis(s.ry);out.lt=s.lt;out.rt=s.rt;
 int x=((s.buttons&PadRight)?1:0)-((s.buttons&PadLeft)?1:0),y=((s.buttons&PadDown)?1:0)-((s.buttons&PadUp)?1:0);
 if(y<0)out.buttons=x<0?7:x>0?1:0;else if(y>0)out.buttons=x<0?5:x>0?3:4;else out.buttons=x<0?6:x>0?2:8;
 const std::uint32_t from[]={PadWest,PadSouth,PadEast,PadNorth,PadLShoulder,PadRShoulder,PadBack,PadStart,PadLStick,PadRStick};
 const std::uint16_t to[]={0x0010,0x0020,0x0040,0x0080,0x0100,0x0200,0x1000,0x2000,0x4000,0x8000};
 for(unsigned i=0;i<10;++i)if(s.buttons&from[i])out.buttons|=to[i];
 if(s.lt>30)out.buttons|=0x0400;
 if(s.rt>30)out.buttons|=0x0800;
 if(s.buttons&PadTouch)out.special|=2;
 return out;
}
bool padAtRest(const PadState& s) {
 return !s.buttons&&s.lt<15&&s.rt<15&&std::abs(static_cast<int>(s.lx))<8000&&std::abs(static_cast<int>(s.ly))<8000&&std::abs(static_cast<int>(s.rx))<8000&&std::abs(static_cast<int>(s.ry))<8000;
}
