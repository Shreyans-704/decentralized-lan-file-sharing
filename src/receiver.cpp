#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "transfer/protocol.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int PORT = 5000;

// Helper — receive exact N bytes
bool recvAll(SOCKET sock, char* buf, int size) {
    int received = 0;
    while (received < size) {
        int r = recv(sock, buf + received, size - received, 0);
        if (r <= 0) return false;
        received += r;
    }
    return true;
}

int main() {
    // ── 1. Initialize Winsock ──────────────────────────────────────────
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[-] WSAStartup failed." << endl;
        return 1;
    }

    // ── 2. Create socket ───────────────────────────────────────────────
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        cerr << "[-] Socket creation failed." << endl;
        WSACleanup();
        return 1;
    }

    // ── 3. Bind ────────────────────────────────────────────────────────
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "[-] Bind failed." << endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // ── 4. Listen ──────────────────────────────────────────────────────
    if (listen(server_fd, 1) == SOCKET_ERROR) {
        cerr << "[-] Listen failed." << endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    cout << "[*] Receiver listening on port " << PORT << endl;
    cout << "[*] Waiting for sender...\n" << endl;

    // ── 5. Accept connection ───────────────────────────────────────────
    sockaddr_in client_addr{};
    int client_len = sizeof(client_addr);

    SOCKET client_fd = accept(server_fd,
                              (sockaddr*)&client_addr,
                              &client_len);

    if (client_fd == INVALID_SOCKET) {
        cerr << "[-] Accept failed." << endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // ✅ FIXED: Get sender IP properly
    string sender_ip = inet_ntoa(client_addr.sin_addr);
    cout << "[+] Connected from: " << sender_ip << "\n" << endl;

    // ── 6. Receive metadata ────────────────────────────────────────────
    FileMetadata meta{};
    if (!recvAll(client_fd, (char*)&meta, sizeof(meta))) {
        cerr << "[-] Failed to receive metadata." << endl;
        return 1;
    }

    cout << "[*] Filename     : " << meta.filename << endl;
    cout << "[*] File size    : " << meta.filesize << " bytes" << endl;
    cout << "[*] Total chunks : " << meta.total_chunks << endl;
    cout << endl;

    // ── 7. Open output file ────────────────────────────────────────────
    string outpath = string("received_") + meta.filename;
    ofstream outfile(outpath, ios::binary);

    if (!outfile.is_open()) {
        cerr << "[-] Cannot create output file." << endl;
        return 1;
    }

    // ── 8. Receive chunks ──────────────────────────────────────────────
    vector<char> buffer(meta.chunk_size);
    uint64_t total_received = 0;

    for (uint32_t i = 0; i < meta.total_chunks; i++) {

        // Receive chunk header
        ChunkHeader header{};
        if (!recvAll(client_fd, (char*)&header, sizeof(header))) {
            cerr << "\n[-] Failed to receive chunk header." << endl;
            break;
        }

        // Receive chunk data
        if (!recvAll(client_fd,
                     buffer.data(),
                     header.chunk_size)) {
            cerr << "\n[-] Failed to receive chunk "
                 << header.chunk_id << endl;
            break;
        }

        // Write to file
        outfile.write(buffer.data(), header.chunk_size);
        total_received += header.chunk_size;

        // Progress
        int percent = static_cast<int>(
            (total_received * 100.0) / meta.filesize);

        cout << "\r[*] Receiving... " << percent << "% ("
             << total_received << "/" << meta.filesize
             << " bytes)" << flush;
    }

    // ── 9. Cleanup ─────────────────────────────────────────────────────
    outfile.close();
    closesocket(client_fd);
    closesocket(server_fd);
    WSACleanup();

    cout << "\n\n[+] File saved as: " << outpath << endl;
    cout << "[+] Total received: " << total_received << " bytes" << endl;

    return 0;
}