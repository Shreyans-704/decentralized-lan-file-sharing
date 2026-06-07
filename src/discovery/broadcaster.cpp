// Shouts "I am here" every 2 seconds via UDP broadcast

#include <iostream>
#include <string>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

const int DISCOVERY_PORT = 5001;
const int BROADCAST_INTERVAL_SEC = 2;

// Get this machine's hostname
string getHostname() {
    char name[256];
    if (gethostname(name, sizeof(name)) == 0)
        return string(name);
    return "Unknown-PC";
}

int main() {
    // ── 1. Initialize Winsock ──────────────────────────────────────────
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[-] WSAStartup failed: " << WSAGetLastError() << endl;
        return 1;
    }

    // ── 2. Create UDP socket ───────────────────────────────────────────
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "[-] socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // ── 3. Enable broadcast ────────────────────────────────────────────
    BOOL broadcast = TRUE;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
                   (char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR) {
        cerr << "[-] setsockopt failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // ── 4. Setup broadcast address ─────────────────────────────────────
    sockaddr_in broadcast_addr{};
    broadcast_addr.sin_family      = AF_INET;
    broadcast_addr.sin_port        = htons(DISCOVERY_PORT);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
    string hostname = getHostname();

    // Message format: "DISCOVER|<hostname>|<tcp_port>|<device_id>"
    string device_id = hostname + "_123"; // simple unique ID (can improve later)
    string message = "DISCOVER|" + hostname + "|5000|" + device_id;

    cout << "[*] Broadcasting as: " << hostname << endl;
    cout << "[*] Broadcasting every " << BROADCAST_INTERVAL_SEC
         << " seconds on port " << DISCOVERY_PORT << endl;
    cout << "[*] Press Ctrl+C to stop\n" << endl;

    // ── 5. Broadcast loop ──────────────────────────────────────────────
    while (true) {
        int sent = sendto(sock,
                          message.c_str(),
                          static_cast<int>(message.size()),
                          0,
                          (sockaddr*)&broadcast_addr,
                          sizeof(broadcast_addr));

        if (sent == SOCKET_ERROR) {
            cerr << "[-] sendto failed: " << WSAGetLastError() << endl;
        } else {
        static int counter = 0;
            counter++;

            if (counter % 3 == 0) {
                cout << "[+] Broadcasting..." << endl;
            }
        }

        Sleep(BROADCAST_INTERVAL_SEC * 1000);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}