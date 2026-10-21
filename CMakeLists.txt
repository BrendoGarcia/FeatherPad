cmake_minimum_required(VERSION 3.24)
project(FeatherPad VERSION 1.1.5 LANGUAGES C CXX RC)
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
FetchContent_Declare(zlibsrc
  URL https://github.com/madler/zlib/archive/refs/tags/v1.3.1.tar.gz
  URL_HASH SHA256=17e88863f3600672ab49182f217281b6fc4d3c762bde361935e436a95214d05c
  SOURCE_SUBDIR _featherpad_populate_only
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(sdl3sdk vigem zlibsrc)
add_library(fp_inflate STATIC ${zlibsrc_SOURCE_DIR}/adler32.c ${zlibsrc_SOURCE_DIR}/crc32.c
  ${zlibsrc_SOURCE_DIR}/inflate.c ${zlibsrc_SOURCE_DIR}/inftrees.c ${zlibsrc_SOURCE_DIR}/inffast.c ${zlibsrc_SOURCE_DIR}/zutil.c)
target_include_directories(fp_inflate PUBLIC ${zlibsrc_SOURCE_DIR})
configure_file(src/app.rc ${CMAKE_CURRENT_BINARY_DIR}/generated/app.rc @ONLY)
add_executable(FeatherPad WIN32 src/main.cpp src/system_metrics.cpp src/library_features.cpp src/controller_profiles.cpp src/storage.cpp src/art.cpp src/ui.cpp src/input.cpp
  src/controller_state.cpp src/gamepad_bridge.cpp src/experience.cpp src/UpdateManager.cpp src/update_common.cpp tests/controller_state_tests.cpp tests/smoke.cpp tests/regressions115.cpp src/update_zip.cpp ${CMAKE_CURRENT_BINARY_DIR}/generated/app.rc)
target_include_directories(FeatherPad PRIVATE src "${sdl3sdk_SOURCE_DIR}/include")
target_compile_features(FeatherPad PRIVATE cxx_std_17)
target_compile_definitions(FeatherPad PRIVATE UNICODE _UNICODE WINVER=0x0601 _WIN32_WINNT=0x0601)
target_link_libraries(FeatherPad PRIVATE ViGEmClient user32 gdi32 shell32 ole32 comdlg32 advapi32 hid windowscodecs uuid dwmapi setupapi cfgmgr32 psapi wlanapi iphlpapi winhttp bcrypt version fp_inflate)
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
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${vigem_SOURCE_DIR}/LICENSE" "$<TARGET_FILE_DIR:FeatherPad>/licenses/ViGEmClient.txt"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${zlibsrc_SOURCE_DIR}/LICENSE" "$<TARGET_FILE_DIR:FeatherPad>/licenses/zlib.txt")
enable_testing()
add_test(NAME native_smoke COMMAND FeatherPad --self-test)
set_tests_properties(native_smoke PROPERTIES TIMEOUT 60)

add_executable(Updater WIN32 src/Updater.cpp src/update_common.cpp)
add_executable(update_tests tests/update_tests.cpp src/update_common.cpp src/update_zip.cpp)
foreach(component Updater update_tests)
  target_include_directories(${component} PRIVATE src)
  target_compile_features(${component} PRIVATE cxx_std_17)
  target_compile_definitions(${component} PRIVATE UNICODE _UNICODE WINVER=0x0601 _WIN32_WINNT=0x0601)
  target_link_libraries(${component} PRIVATE bcrypt shell32 version)
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
target_link_libraries(update_tests PRIVATE fp_inflate)
foreach(component FeatherPad Updater update_tests)
  target_compile_definitions(${component} PRIVATE FEATHERPAD_VERSION="${PROJECT_VERSION}")
endforeach()
add_test(NAME update_integrity_and_rollback COMMAND update_tests)
set_tests_properties(update_integrity_and_rollback PROPERTIES TIMEOUT 60)

