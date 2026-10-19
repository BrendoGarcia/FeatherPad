#include "controller_state.h"
#include <string>

int controllerStateSelfTest(std::string& log) {
 int failed=0;auto check=[&](bool ok,const char* name){log+=(ok?"PASS ":"FAIL ");log+=name;log+='\n';if(!ok)++failed;};
 check(resolveControllerMode(ControllerMode::Auto,ControllerMode::Xbox)==ControllerMode::Xbox&&resolveControllerMode(ControllerMode::Auto,ControllerMode::PlayStation)==ControllerMode::PlayStation,"game defaults follow the selected global controller format");
 check(resolveControllerMode(ControllerMode::PlayStation,ControllerMode::Xbox)==ControllerMode::PlayStation&&resolveControllerMode(ControllerMode::Native,ControllerMode::Xbox)==ControllerMode::Native,"explicit per-game PlayStation and native preferences override the default");
 PadState s;const auto neutral=toPlayStation(s);
 check(neutral.lx==128&&neutral.ly==128&&neutral.rx==128&&neutral.ry==128&&neutral.buttons==8,"neutral PlayStation report has centered sticks and released D-pad");
 s.buttons=PadSouth|PadEast|PadWest|PadNorth;check(toXbox(s).buttons==0xf000&&(toPlayStation(s).buttons&0xf0)==0xf0,"all face buttons translate by position into both output formats");
 const std::uint32_t from[]={PadStart,PadBack,PadLStick,PadRStick,PadLShoulder,PadRShoulder};
 const std::uint16_t xbox[]={0x0010,0x0020,0x0040,0x0080,0x0100,0x0200},sony[]={0x2000,0x1000,0x4000,0x8000,0x0100,0x0200};
 bool buttons=true;for(int i=0;i<6;++i){s={};s.buttons=from[i];buttons=buttons&&toXbox(s).buttons==xbox[i]&&(toPlayStation(s).buttons&0xfff0)==sony[i];}check(buttons,"shoulders, stick clicks, start and back translate correctly");
 s={};s.buttons=PadUp|PadRight;check((toPlayStation(s).buttons&15)==1&&toXbox(s).buttons==9,"diagonal D-pad translates to a Sony hat and Xbox button bits");
 s.buttons=PadLeft|PadDown;check((toPlayStation(s).buttons&15)==5,"down-left D-pad has the correct Sony hat value");
 s.buttons=PadLeft|PadRight|PadUp|PadDown;check((toPlayStation(s).buttons&15)==8,"opposing D-pad directions cancel for PlayStation output");
 s={};s.lx=-32768;s.rx=32767;s.ly=-32768;s.ry=32767;s.lt=127;s.rt=255;
 check(toXbox(s).lx==-32768&&toXbox(s).rx==32767&&toXbox(s).ly==32767&&toXbox(s).ry==-32767,"Xbox axes preserve range and invert Y without overflow");
 const auto ps=toPlayStation(s);check(ps.lx==0&&ps.rx==255&&ps.ly==0&&ps.ry==255&&ps.lt==127&&ps.rt==255&&(ps.buttons&0xc00)==0xc00,"PlayStation axes and independent analog triggers cover their full range");
 s={};s.buttons=PadGuide;check(toXbox(s).buttons==0&&toPlayStation(s).special==0,"launcher guide button is not forwarded to system overlays");
 s={};check(padAtRest(s),"neutral input can release the launcher handoff guard");s.buttons=PadSouth;check(!padAtRest(s),"held launch button cannot leak into the game");s={};s.lx=9000;check(!padAtRest(s),"held analog stick keeps the handoff guard active");
 return failed;
}
#ifdef FEATHERPAD_STATE_TEST_MAIN
#include <iostream>
int main(){std::string log;int result=controllerStateSelfTest(log);std::cout<<log;return result?1:0;}
#endif
