#include <iostream>
#include <fstream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <output_file>" << std::endl;
        return 1;
    }

    const char* output_file = argv[1];
    const int PORT = 5000;
    const int BUFFER_SIZE = 4096;

    // ── 1. Initialize Winsock ──────────────────────────────────────────
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    // ── 2. Create listening socket ─────────────────────────────────────
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // ── 3. Bind ────────────────────────────────────────────────────────
    sockaddr_in server_addr{};
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;  // accept from any IP

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // ── 4. Listen ──────────────────────────────────────────────────────
    if (listen(server_fd, 1) == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "[*] Receiver listening on port " << PORT << " ..." << std::endl;

    // ── 5. Accept connection ───────────────────────────────────────────
    sockaddr_in client_addr{};
    int client_len = sizeof(client_addr);

    SOCKET client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "[+] Connected from: "
          << inet_ntoa(client_addr.sin_addr) << std::endl;

    // ── 6. Open output file ────────────────────────────────────────────
    std::ofstream file(output_file, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[-] Failed to open output file: " << output_file << std::endl;
        closesocket(client_fd);
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // ── 7. Receive file data ───────────────────────────────────────────
    char buffer[BUFFER_SIZE];
    int bytes_received;
    long long total = 0;

        while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        file.write(buffer, bytes_received);
        total += bytes_received;
        std::cout << "\r[+] Received: " << total << " bytes" << std::flush;
    }

    if (bytes_received == 0) {
        cout << "\n[+] Sender disconnected normally." << endl;
    }
    else if (bytes_received == SOCKET_ERROR) {
        std::cerr << "\n[-] recv failed: " << WSAGetLastError() << std::endl;
    }

    // ── 8. Cleanup ─────────────────────────────────────────────────────
    file.close();
    closesocket(client_fd);
    closesocket(server_fd);
    WSACleanup();

    std::cout << "\n[+] File saved to: " << output_file << std::endl;
    std::cout << "[+] Total bytes received: " << total << std::endl;

    return 0;
}