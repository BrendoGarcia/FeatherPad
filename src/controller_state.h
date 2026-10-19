#pragma once
#include <cstdint>
#include <string>

// Controller-independent button positions, not a physical device's button numbers.
enum PadButton : std::uint32_t {
 PadSouth=1u<<0,PadEast=1u<<1,PadWest=1u<<2,PadNorth=1u<<3,
 PadBack=1u<<4,PadGuide=1u<<5,PadStart=1u<<6,PadLStick=1u<<7,PadRStick=1u<<8,
 PadLShoulder=1u<<9,PadRShoulder=1u<<10,PadUp=1u<<11,PadDown=1u<<12,
 PadLeft=1u<<13,PadRight=1u<<14,PadTouch=1u<<15
};
enum class ControllerMode { Auto=0, Native=1, Xbox=2, PlayStation=3 };
struct PadState {
 std::uint32_t buttons=0;
 std::int16_t lx=0,ly=0,rx=0,ry=0; // Y is negative upward, matching SDL / Sony.
 std::uint8_t lt=0,rt=0;
};
struct XboxState { std::uint16_t buttons=0;std::uint8_t lt=0,rt=0;std::int16_t lx=0,ly=0,rx=0,ry=0; };
struct PlayStationState { std::uint8_t lx=128,ly=128,rx=128,ry=128;std::uint16_t buttons=8;std::uint8_t special=0,lt=0,rt=0; };
ControllerMode resolveControllerMode(ControllerMode perGame,ControllerMode defaultMode);
XboxState toXbox(const PadState& state);
PlayStationState toPlayStation(const PadState& state);
bool padAtRest(const PadState& state);

int controllerStateSelfTest(std::string& log);
