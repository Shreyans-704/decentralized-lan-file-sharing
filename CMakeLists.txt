cmake_minimum_required(VERSION 3.10)
project(DecentralizedLANFileSharing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(sender   src/sender.cpp)
add_executable(receiver src/receiver.cpp)

# Required on Windows for Winsock2 (sockets)
target_link_libraries(sender   PRIVATE ws2_32)
target_link_libraries(receiver PRIVATE ws2_32)