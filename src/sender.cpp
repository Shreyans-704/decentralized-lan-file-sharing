#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "transfer/protocol.h"
#include "hasher.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int PORT = 5000;

// ── Progress bar ─────────────────────────────────────
void printProgress(uint32_t current, uint32_t total, double speed_mbps) {
    int percent = (int)((current * 100.0) / total);
    int bars    = percent / 2;

    cout << "\r[";
    for (int i = 0; i < 50; i++)
        cout << (i < bars ? '#' : '.');

    cout << "] " << percent << "% | "
         << speed_mbps << " MB/s  " << flush;
}

// Send exact bytes
bool sendAll(SOCKET sock, const char* buf, int size) {
    int sent = 0;
    while (sent < size) {
        int r = send(sock, buf + sent, size - sent, 0);
        if (r == SOCKET_ERROR) return false;
        sent += r;
    }
    return true;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        cerr << "Usage: " << argv[0]
             << " <receiver_ip> <file_path>" << endl;
        return 1;
    }

    const char* receiver_ip = argv[1];
    const char* file_path   = argv[2];

    // ── 1. Winsock ──────────────────────────────────
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[-] WSAStartup failed." << endl;
        return 1;
    }

    // ── 2. Open file ────────────────────────────────
    ifstream file(file_path, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "[-] Cannot open file: " << file_path << endl;
        WSACleanup();
        return 1;
    }

    uint64_t filesize = file.tellg();
    file.seekg(0);

    uint32_t total_chunks =
        (uint32_t)((filesize + CHUNK_SIZE_BYTES - 1) / CHUNK_SIZE_BYTES);

    string filepath_str(file_path);
    string filename = filepath_str.substr(
        filepath_str.find_last_of("/\\") + 1);

    // ── 3. SHA-256 ──────────────────────────────────
    cout << "[*] Computing SHA-256..." << endl;
    string file_hash = SHA256::hashFile(file_path);
    cout << "[+] Hash: " << file_hash << endl;

    cout << "[*] File   : " << filename << endl;
    cout << "[*] Size   : " << filesize << " bytes" << endl;
    cout << "[*] Chunks : " << total_chunks
         << " x " << (CHUNK_SIZE_BYTES / 1024) << " KB" << endl;

    // ── 4. Socket + connect ─────────────────────────
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in receiver_addr{};
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port   = htons(PORT);

    // ✅ FIX (Windows-safe)
    if (inet_addr(receiver_ip) == INADDR_NONE) {
        cerr << "[-] Invalid IP address." << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    receiver_addr.sin_addr.s_addr = inet_addr(receiver_ip);

    cout << "[*] Connecting to " << receiver_ip << "..." << endl;

    if (connect(sock, (sockaddr*)&receiver_addr,
                sizeof(receiver_addr)) == SOCKET_ERROR) {

        cerr << "[-] connect failed: "
             << WSAGetLastError() << endl;

        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "[+] Connected!\n" << endl;

    // ── 5. Receive resume info ──────────────────────
    ResumeRequest resume{};
    recv(sock, (char*)&resume, sizeof(resume), 0);

    vector<bool> already_received(total_chunks, false);

    for (uint32_t i = 0; i < resume.received_count; i++) {
        if (resume.received_ids[i] < total_chunks)
            already_received[resume.received_ids[i]] = true;
    }

    if (resume.received_count > 0) {
        cout << "[*] Resuming — skipping "
             << resume.received_count << " chunks\n" << endl;
    }

    // ── 6. Send metadata ────────────────────────────
    FileMetadata meta{};

    strncpy(meta.filename, filename.c_str(),
            sizeof(meta.filename) - 1);
    meta.filename[sizeof(meta.filename) - 1] = '\0';

    meta.filesize     = filesize;
    meta.total_chunks = total_chunks;
    meta.chunk_size   = CHUNK_SIZE_BYTES;

    sendAll(sock, (char*)&meta, sizeof(meta));

    // ── 7. Send chunks ──────────────────────────────
    vector<char> buffer(CHUNK_SIZE_BYTES);

    uint64_t total_sent  = 0;
    uint32_t chunks_sent = 0;

    auto start_time = chrono::steady_clock::now();

    for (uint32_t i = 0; i < total_chunks; i++) {

        size_t offset = (size_t)i * CHUNK_SIZE_BYTES;

        uint32_t to_read = (uint32_t)
            min((uint64_t)CHUNK_SIZE_BYTES, filesize - offset);

        file.seekg(offset);
        file.read(buffer.data(), to_read);

        uint32_t bytes_read = (uint32_t)file.gcount();

        // Skip already received
        if (already_received[i]) {
            total_sent += bytes_read;
            chunks_sent++;
            continue;
        }

        ChunkHeader header{ i, bytes_read };

        sendAll(sock, (char*)&header, sizeof(header));
        sendAll(sock, buffer.data(), bytes_read);

        total_sent += bytes_read;
        chunks_sent++;

        // Speed
        auto now = chrono::steady_clock::now();
        double elapsed =
            chrono::duration<double>(now - start_time).count();

        double speed = (elapsed > 0)
            ? (total_sent / (1024.0 * 1024.0)) / elapsed
            : 0.0;

        speed = (int)(speed * 100) / 100.0;

        printProgress(chunks_sent, total_chunks, speed);
    }

    // ── 8. Send hash ────────────────────────────────
    HashPacket hp{};

    strncpy(hp.hash, file_hash.c_str(),
            sizeof(hp.hash) - 1);
    hp.hash[sizeof(hp.hash) - 1] = '\0';
    sendAll(sock, (char*)&hp, sizeof(hp));

    cout << "\n\n[+] File sent!" << endl;
    cout << "[+] SHA-256 sent: " << file_hash << endl;

    // ── 9. Cleanup ─────────────────────────────────
    file.close();
    closesocket(sock);
    WSACleanup();

    return 0;
}