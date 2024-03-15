# CMakeList.txt : CMake project for neural-souls, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("dinput8")

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

add_definitions(-DUNICODE -D_UNICODE)

#SET(SPOUT_BUILD_CMT ON)
SET(SPOUT_BUILD_SPOUTDX ON)

add_subdirectory(Spout2)

add_library(dinput8 SHARED dllmain.cpp logging.cpp DSIIIDX.cpp DSIII.cpp kiero.cpp ScreenGrab11.cpp)

#find_package(Spout CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(utf8cpp CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)

target_link_libraries(dinput8 PRIVATE Spout SpoutDX minhook::minhook spdlog::spdlog spdlog::spdlog_header_only utf8cpp dxguid)

# TODO: Add tests and install targets if needed.


add_custom_command(TARGET dinput8 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E 
	copy $<TARGET_FILE:dinput8> "C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game/dinput8.dll"
)