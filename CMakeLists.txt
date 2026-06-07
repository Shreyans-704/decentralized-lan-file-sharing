cmake_minimum_required(VERSION 3.10)
project(DecentralizedLANFileSharing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Phase 1
add_executable(sender   src/sender.cpp)
add_executable(receiver src/receiver.cpp)

# Phase 2
add_executable(broadcaster src/discovery/broadcaster.cpp)
add_executable(listener    src/discovery/listener.cpp)

# Link Winsock2 to all
target_link_libraries(sender      PRIVATE ws2_32)
target_link_libraries(receiver    PRIVATE ws2_32)
target_link_libraries(broadcaster PRIVATE ws2_32 iphlpapi)
target_link_libraries(listener    PRIVATE ws2_32)

find_package(Threads REQUIRED)