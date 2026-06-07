#include <iostream>
#include <fstream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <receiver_ip> <file_path>" << std::endl;
        return 1;
    }

    const char* receiver_ip = argv[1];
    const char* file_path   = argv[2];
    const int PORT          = 5000;
    const int BUFFER_SIZE   = 4096;

    // ── 1. Initialize Winsock ──────────────────────────────────────────
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[-] WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    // ── 2. Create socket ───────────────────────────────────────────────
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[-] socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // ── 3. Connect to receiver ─────────────────────────────────────────
    sockaddr_in receiver_addr{};
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port   = htons(PORT);

        if (inet_addr(receiver_ip) == INADDR_NONE) {
        cout << "Invalid address\n";
        return 1;
    }
receiver_addr.sin_addr.s_addr = inet_addr(receiver_ip);

    std::cout << "[*] Connecting to " << receiver_ip << ":" << PORT << " ..." << std::endl;

    if (connect(sock, (sockaddr*)&receiver_addr, sizeof(receiver_addr)) == SOCKET_ERROR) {
        std::cerr << "[-] connect failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "[+] Connected to receiver." << std::endl;

    // ── 4. Open file ───────────────────────────────────────────────────
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[-] Cannot open file: " << file_path << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    long long file_size = file.tellg();
    file.seekg(0);

    std::cout << "[*] Sending file: " << file_path << std::endl;
    std::cout << "[*] File size   : " << file_size << " bytes" << std::endl;

    // ── 5. Send file data ──────────────────────────────────────────────
    char buffer[BUFFER_SIZE];
    long long total_sent = 0;

    while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
        int to_send = static_cast<int>(file.gcount());
        int bytes_sent = send(sock, buffer, to_send, 0);

        if (bytes_sent == SOCKET_ERROR) {
            std::cerr << "\n[-] send failed: " << WSAGetLastError() << std::endl;
            break;
        }

        total_sent += bytes_sent;

        // Progress
        int progress = static_cast<int>((total_sent * 100) / file_size);
        std::cout << "\r[+] Progress: " << progress << "% ("
                  << total_sent << "/" << file_size << " bytes)" << std::flush;
    }

    // ── 6. Cleanup ─────────────────────────────────────────────────────
    file.close();
    closesocket(sock);
    WSACleanup();

    std::cout << "\n[+] File sent successfully!" << std::endl;
    std::cout << "[+] Total bytes sent: " << total_sent << std::endl;

    return 0;
}