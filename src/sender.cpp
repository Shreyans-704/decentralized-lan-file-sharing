#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "transfer/protocol.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int    PORT       = 5000;
const size_t CHUNK_SIZE = 512 * 1024; // 512 KB

// ── Progress bar ───────────────────────────────────────────────────────
void printProgress(uint32_t current, uint32_t total,
                   double speed_mbps) {

    int percent = static_cast<int>((current * 100.0) / total);
    int bars    = percent / 2;

    cout << "\r[";
    for (int i = 0; i < 50; i++)
        cout << (i < bars ? '#' : '.');

    cout << "] " << percent << "% | "
         << speed_mbps << " MB/s  " << flush;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        cerr << "Usage: " << argv[0]
             << " <receiver_ip> <file_path>" << endl;
        return 1;
    }

    const char* receiver_ip = argv[1];
    const char* file_path   = argv[2];

    // ── 1. Initialize Winsock ──────────────────────────────────────────
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[-] WSAStartup failed." << endl;
        return 1;
    }

    // ── 2. Open file ───────────────────────────────────────────────────
    ifstream file(file_path, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "[-] Cannot open file: " << file_path << endl;
        WSACleanup();
        return 1;
    }

    uint64_t filesize = file.tellg();
    file.seekg(0);

    uint32_t total_chunks = static_cast<uint32_t>(
        (filesize + CHUNK_SIZE - 1) / CHUNK_SIZE);

    string filepath_str(file_path);
    string filename = filepath_str.substr(
        filepath_str.find_last_of("/\\") + 1);

    cout << "[*] File   : " << filename << endl;
    cout << "[*] Size   : " << filesize << " bytes" << endl;
    cout << "[*] Chunks : " << total_chunks
         << " x " << (CHUNK_SIZE / 1024) << " KB\n" << endl;

    // ── 3. Create socket ───────────────────────────────────────────────
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "[-] socket failed." << endl;
        WSACleanup();
        return 1;
    }

    // ── 4. Setup receiver address ──────────────────────────────────────
    sockaddr_in receiver_addr{};
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port   = htons(PORT);

    // ✅ FIX: Windows-safe IP conversion
    if (inet_addr(receiver_ip) == INADDR_NONE) {
        cerr << "[-] Invalid IP address." << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    receiver_addr.sin_addr.s_addr = inet_addr(receiver_ip);

    cout << "[*] Connecting to " << receiver_ip << "..." << endl;

    // ── 5. Connect ─────────────────────────────────────────────────────
    if (connect(sock,
                (sockaddr*)&receiver_addr,
                sizeof(receiver_addr)) == SOCKET_ERROR) {

        cerr << "[-] connect failed: "
             << WSAGetLastError() << endl;

        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "[+] Connected!\n" << endl;

    // ── 6. Send metadata ───────────────────────────────────────────────
    FileMetadata meta{};

    strncpy(meta.filename, filename.c_str(),
            sizeof(meta.filename) - 1);
    meta.filename[sizeof(meta.filename) - 1] = '\0';

    meta.filesize     = filesize;
    meta.total_chunks = total_chunks;
    meta.chunk_size   = static_cast<uint32_t>(CHUNK_SIZE);

    send(sock, (char*)&meta, sizeof(meta), 0);

    // ── 7. Send chunks ─────────────────────────────────────────────────
    vector<char> buffer(CHUNK_SIZE);
    uint64_t total_sent = 0;

    auto start_time = chrono::steady_clock::now();

    for (uint32_t i = 0; i < total_chunks; i++) {

        file.read(buffer.data(), CHUNK_SIZE);
        uint32_t bytes_read = static_cast<uint32_t>(file.gcount());

        // Send header
        ChunkHeader header{ i, bytes_read };
        send(sock, (char*)&header, sizeof(header), 0);

        // Send data safely
        uint32_t sent = 0;
        while (sent < bytes_read) {

            int result = send(sock,
                              buffer.data() + sent,
                              bytes_read - sent, 0);

            if (result == SOCKET_ERROR) {
                cerr << "\n[-] send failed: "
                     << WSAGetLastError() << endl;
                goto cleanup;
            }

            sent += result;
        }

        total_sent += bytes_read;

        // Speed calc
        auto now = chrono::steady_clock::now();
        double elapsed = chrono::duration<double>(
            now - start_time).count();

        double speed = (elapsed > 0)
            ? (total_sent / (1024.0 * 1024.0)) / elapsed
            : 0.0;

        speed = static_cast<int>(speed * 100) / 100.0;

        printProgress(i + 1, total_chunks, speed);
    }

    cout << "\n\n[+] File sent successfully!" << endl;
    cout << "[+] Total sent: " << total_sent << " bytes" << endl;

cleanup:
    file.close();
    closesocket(sock);
    WSACleanup();
    return 0;
}