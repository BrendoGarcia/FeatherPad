cmake_minimum_required(VERSION 3.24)
project(FeatherPad VERSION 2.2.0 LANGUAGES C CXX RC)
if(NOT WIN32)
  message(FATAL_ERROR "FeatherPad usa APIs nativas do Windows. Use Windows ou toolchain MinGW.")
endif()
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
include(FetchContent)
# Exact versions and archive hashes: no unpinned download at build time.
FetchContent_Declare(sdl3sdk
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-devel-3.4.18-VC.zip
  URL_HASH SHA256=78d84602ae616cfe26b33a73b7d3b9b6b56e8a6650e53d92c9c6ca938f747acd
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(vigem
  URL https://github.com/nefarius/ViGEmClient/archive/b66d02d57e32cc8595369c53418b843e958649b4.tar.gz
  URL_HASH SHA256=c2bc824cc96ed18cf0620b734ae552474f107e7e680485e5c0cb63e675d9f72a
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(ViGEmClient_DLL OFF CACHE BOOL "Static client; no separate emulation process" FORCE)
FetchContent_MakeAvailable(sdl3sdk vigem)
add_executable(FeatherPad WIN32 src/main.cpp src/storage.cpp src/art.cpp src/ui.cpp src/input.cpp
  src/controller_state.cpp src/gamepad_bridge.cpp src/experience.cpp src/UpdateManager.cpp src/update_common.cpp tests/controller_state_tests.cpp tests/smoke.cpp src/app.rc)
target_include_directories(FeatherPad PRIVATE src "${sdl3sdk_SOURCE_DIR}/include")
target_compile_features(FeatherPad PRIVATE cxx_std_17)
target_compile_definitions(FeatherPad PRIVATE UNICODE _UNICODE WINVER=0x0601 _WIN32_WINNT=0x0601)
target_link_libraries(FeatherPad PRIVATE ViGEmClient user32 gdi32 shell32 ole32 comdlg32 advapi32 hid windowscodecs uuid dwmapi setupapi cfgmgr32 winhttp bcrypt)
if(MSVC)
  target_compile_options(FeatherPad PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/W4;/O1;/utf-8>")
else()
  target_compile_options(FeatherPad PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Os>")
  target_link_options(FeatherPad PRIVATE -municode -static -static-libgcc -static-libstdc++ -s)
endif()
add_custom_command(TARGET FeatherPad POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${sdl3sdk_SOURCE_DIR}/lib/x64/SDL3.dll" "$<TARGET_FILE_DIR:FeatherPad>/SDL3.dll"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:FeatherPad>/licenses"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${sdl3sdk_SOURCE_DIR}/LICENSE.txt" "$<TARGET_FILE_DIR:FeatherPad>/licenses/SDL3.txt"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${vigem_SOURCE_DIR}/LICENSE" "$<TARGET_FILE_DIR:FeatherPad>/licenses/ViGEmClient.txt")
enable_testing()
add_test(NAME native_smoke COMMAND FeatherPad --self-test)
set_tests_properties(native_smoke PROPERTIES TIMEOUT 60)

add_executable(Updater WIN32 src/Updater.cpp src/update_common.cpp)
add_executable(update_tests tests/update_tests.cpp src/update_common.cpp)
foreach(component Updater update_tests)
  target_include_directories(${component} PRIVATE src)
  target_compile_features(${component} PRIVATE cxx_std_17)
  target_compile_definitions(${component} PRIVATE UNICODE _UNICODE WINVER=0x0601 _WIN32_WINNT=0x0601)
  target_link_libraries(${component} PRIVATE bcrypt shell32)
  if(MSVC)
    target_compile_options(${component} PRIVATE /W4 /utf-8)
  else()
    target_compile_options(${component} PRIVATE -Wall -Wextra)
    target_link_options(${component} PRIVATE -static -static-libgcc -static-libstdc++)
    if(component STREQUAL "Updater")
      target_link_options(${component} PRIVATE -municode)
    endif()
  endif()
endforeach()
add_test(NAME update_integrity_and_rollback COMMAND update_tests)
set_tests_properties(update_integrity_and_rollback PROPERTIES TIMEOUT 60)
