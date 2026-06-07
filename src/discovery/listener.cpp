// Listens for UDP broadcasts and maintains a live peer list

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int DISCOVERY_PORT = 5001;

struct Peer {
    string name;
    string ip;
    int    port;
};

// Parse "DISCOVER|hostname|port" → Peer
bool parseMessage(const string& msg,
                  const string& senderIP,
                  Peer& peer) {

    if (msg.rfind("DISCOVER|", 0) != 0)
        return false;

    istringstream ss(msg);
    string token;

    getline(ss, token, '|');      // DISCOVER
    getline(ss, peer.name, '|');  // hostname
    getline(ss, token, '|');      // port

    peer.ip   = senderIP;
    peer.port = stoi(token);

    return true;
}

// Print current peer list
void printPeerList(const map<string, Peer>& peers) {
    cout << "\n========= Available Peers =========\n";

    if (peers.empty()) {
        cout << "  (none found yet)\n";
    } else {
        int i = 1;
        for (auto& p : peers) {
            cout << "  " << i++ << ". "
                 << p.second.name << " ("
                 << p.second.ip   << ":"
                 << p.second.port << ")\n";
        }
    }

    cout << "===================================\n\n";
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

    // ── 3. Allow multiple sockets on same port ─────────────────────────
    BOOL reuse = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
               (char*)&reuse, sizeof(reuse));

    // ── 4. Bind to discovery port ──────────────────────────────────────
    sockaddr_in local_addr{};
    local_addr.sin_family      = AF_INET;
    local_addr.sin_port        = htons(DISCOVERY_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&local_addr, sizeof(local_addr)) == SOCKET_ERROR) {
        cerr << "[-] bind failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "[*] Listener started on port " << DISCOVERY_PORT << endl;
    cout << "[*] Waiting for peers...\n" << endl;

    // ── 5. Peer list ───────────────────────────────────────────────────
    map<string, Peer> peers;

    // ── 6. Listen loop ─────────────────────────────────────────────────
    char buffer[512];
    sockaddr_in sender_addr{};
    int sender_len = sizeof(sender_addr);

    while (true) {
        int bytes = recvfrom(sock,
                             buffer,
                             sizeof(buffer) - 1,
                             0,
                             (sockaddr*)&sender_addr,
                             &sender_len);

        if (bytes == SOCKET_ERROR) {
            cerr << "[-] recvfrom failed: " << WSAGetLastError() << endl;
            continue;
        }

        buffer[bytes] = '\0';
        string message(buffer);

        // 🔥 FIX: use inet_ntoa instead of inet_ntop
        string sender_ip = inet_ntoa(sender_addr.sin_addr);

        Peer peer;
        if (!parseMessage(message, sender_ip, peer))
            continue;

        // New peer
        bool isNew = (peers.find(peer.ip) == peers.end());

            peers[peer.ip] = peer;

            if (isNew) {
                cout << "[+] New peer found: "
                    << peer.name << " (" << peer.ip << ")\n";
                printPeerList(peers);
}
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}