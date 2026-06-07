#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "transfer/protocol.h"
#include "hasher.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")

const int PORT = 5000;
const string LOG_FILE = "chunks.log";

// Receive exact bytes
bool recvAll(SOCKET sock, char* buf, int size) {
    int received = 0;
    while (received < size) {
        int r = recv(sock, buf + received, size - received, 0);
        if (r <= 0) return false;
        received += r;
    }
    return true;
}

// Load chunk IDs (resume)
set<uint32_t> loadReceivedChunks() {
    set<uint32_t> chunks;
    ifstream log(LOG_FILE);
    if (!log.is_open()) return chunks;

    uint32_t id;
    while (log >> id)
        chunks.insert(id);

    return chunks;
}

// Log chunk
void logChunk(uint32_t id) {
    ofstream log(LOG_FILE, ios::app);
    log << id << "\n";
}

// Clear log
void clearLog() {
    ofstream log(LOG_FILE, ios::trunc);
}

int main() {

    // ── 1. Winsock init ─────────────────────────────
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[-] WSAStartup failed." << endl;
        return 1;
    }

    // ── 2. Server setup ─────────────────────────────
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    cout << "[*] Receiver listening on port " << PORT << endl;
    cout << "[*] Waiting for sender...\n" << endl;

    // ── 3. Accept ───────────────────────────────────
    sockaddr_in client_addr{};
    int client_len = sizeof(client_addr);

    SOCKET client_fd = accept(server_fd,
                              (sockaddr*)&client_addr,
                              &client_len);

    string sender_ip = inet_ntoa(client_addr.sin_addr);

    cout << "[+] Connected from: "
        << sender_ip << "\n" << endl;

    // ── 4. Resume state ─────────────────────────────
    set<uint32_t> received_chunks = loadReceivedChunks();

    ResumeRequest resume{};
    resume.received_count = (uint32_t)received_chunks.size();

    uint32_t idx = 0;
    for (auto id : received_chunks)
        resume.received_ids[idx++] = id;

    send(client_fd, (char*)&resume, sizeof(resume), 0);

    if (!received_chunks.empty()) {
        cout << "[*] Resuming — already have "
             << received_chunks.size()
             << " chunks\n" << endl;
    }

    // ── 5. Metadata ─────────────────────────────────
    FileMetadata meta{};
    if (!recvAll(client_fd, (char*)&meta, sizeof(meta))) {
        cerr << "[-] Failed to receive metadata." << endl;
        return 1;
    }

    cout << "[*] Filename     : " << meta.filename << endl;
    cout << "[*] File size    : " << meta.filesize << " bytes" << endl;
    cout << "[*] Total chunks : " << meta.total_chunks << "\n" << endl;

    // ── 6. File open ────────────────────────────────
    string outpath = string("received_") + meta.filename;

    ofstream outfile(outpath, ios::binary);
    if (!outfile.is_open()) {
        cerr << "[-] Cannot create output file." << endl;
        return 1;
    }

    // ── 7. Receive chunks ───────────────────────────
    vector<char> buffer(meta.chunk_size);

    uint64_t total_received = 0;
    uint32_t chunks_done    = (uint32_t)received_chunks.size();

    uint32_t chunks_to_receive =
        meta.total_chunks - (uint32_t)received_chunks.size();

    for (uint32_t i = 0; i < chunks_to_receive; i++) {

        ChunkHeader header{};
        if (!recvAll(client_fd, (char*)&header, sizeof(header))) {
            cerr << "\n[-] Connection lost at chunk "
                 << chunks_done << endl;

            cerr << "[*] Progress saved — restart to resume." << endl;

            outfile.close();
            closesocket(client_fd);
            closesocket(server_fd);
            WSACleanup();
            return 1;
        }

        if (!recvAll(client_fd,
                     buffer.data(),
                     header.chunk_size)) {
            cerr << "\n[-] Failed receiving chunk data "
                 << header.chunk_id << endl;
            break;
        }

        // Write at correct position
        outfile.seekp(header.chunk_id * meta.chunk_size);
        outfile.write(buffer.data(), header.chunk_size);

        logChunk(header.chunk_id);
        received_chunks.insert(header.chunk_id);

        total_received += header.chunk_size;
        chunks_done++;

        int percent = (int)((chunks_done * 100.0) / meta.total_chunks);

        cout << "\r[*] Receiving... " << percent << "% ("
             << total_received << "/" << meta.filesize
             << " bytes)" << flush;
    }

    // ── 8. SHA-256 verify ───────────────────────────
    HashPacket hp{};
    recvAll(client_fd, (char*)&hp, sizeof(hp));

    outfile.close();

    cout << "\n\n[*] Verifying integrity..." << endl;
    cout << "[*] Sender hash  : " << hp.hash << endl;

    string received_hash = SHA256::hashFile(outpath);
    cout << "[*] Received hash: " << received_hash << endl;

    if (received_hash == string(hp.hash)) {
        cout << "\n[+] ✅ Integrity verified — file is perfect!" << endl;
        clearLog();
    } else {
        cout << "\n[-] ❌ Integrity check FAILED — file corrupted!" << endl;
    }

    // ── 9. Cleanup ─────────────────────────────────
    closesocket(client_fd);
    closesocket(server_fd);
    WSACleanup();

    cout << "[+] File saved as : " << outpath << endl;
    cout << "[+] Total received: " << total_received << " bytes" << endl;

    return 0;
}