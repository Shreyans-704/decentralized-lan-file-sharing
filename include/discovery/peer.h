#pragma once
#include <string>

struct Peer {
    std::string name;   // e.g. "Shreyans-PC"
    std::string ip;     // e.g. "192.168.1.5"
    int         port;   // TCP port for file transfer
};