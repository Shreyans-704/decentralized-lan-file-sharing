# 🚀 Decentralized LAN File Sharing
A C++ based peer-to-peer file transfer system that enables direct file sharing between devices over a local network using TCP sockets and UDP-based peer discovery.

---

## 📌 Overview
This project implements a **decentralized file sharing system** where devices discover each other automatically and communicate directly over a LAN without any central server.

It demonstrates:
* Low-level **socket programming**
* **TCP-based reliable data transfer**
* **UDP-based peer discovery**
* **Chunked file streaming**
* Real-world networking concepts

---

## 🏗️ Project Structure
<img width="567" height="320" alt="image" src="https://github.com/user-attachments/assets/4e4e1150-c265-42b2-bb04-69076c546134" />


---
## SYSTEM ARCHITECTURE : - 
<img width="1536" height="1024" alt="image" src="https://github.com/user-attachments/assets/e5f8a44a-bd1a-4df0-9409-5764430a2ea1" />

## ⚙️ Building the Project

### 🔧 Prerequisites
* C++17 compatible compiler (MinGW / GCC / Clang)
* CMake 3.10+
* Windows (Winsock2)

---

### 🛠️ Build Steps
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Produces 4 executables:
build/sender.exe
build/receiver.exe
build/broadcaster.exe
build/listener.exe

---

## ▶️ Usage

### 📡 Phase 2: Peer Discovery (Run First)

#### Terminal 1 — Start Listener
```bash
build\listener.exe
```
Output:

[] Listener started on port 5001
[] Waiting for peers...

#### Terminal 2 — Start Broadcaster
```bash
build\broadcaster.exe
```
Output:
[] Broadcasting as: Shreyans-PC
[] Broadcasting every 2 seconds on port 5001
[+] Broadcast sent: DISCOVER|Shreyans-PC|5000

#### Listener detects peer:


---

### 📁 Phase 1: File Transfer

#### Step 1: Start Receiver
```bash
build\receiver.exe output.txt
```
Output:
[*] Receiver listening on port 5000 ...

#### Step 2: Run Sender
```bash
build\sender.exe <receiver_ip> testfile.txt
```
Example:
```bash
build\sender.exe 127.0.0.1 testfile.txt
```

---

## 🧪 Example Output

### Receiver:
[*] Receiver listening on port 5000 ...
[+] Connected from: 127.0.0.1
[+] Received: 28 bytes
[+] File saved to: output.txt

### Sender:
[] Connecting to 127.0.0.1:5000 ...
[+] Connected to receiver.
[] Sending file: testfile.txt
[+] Progress: 100%
[+] File sent successfully!

---

## 🔄 How It Works

### Full Flow
Both devices start broadcaster + listener
↓
UDP broadcast → "DISCOVER|hostname|port"
↓
Listener builds live peer list
↓
User selects target device
↓
TCP connection established
↓
File sent in chunks
↓
Receiver reassembles file
↓
Done ✅

### Discovery Layer (UDP)
* Each device broadcasts presence every **2 seconds**
* Broadcast sent to `255.255.255.255:5001`
* Listener maintains live `map<IP → Peer>`
* Message format: `DISCOVER|<hostname>|<tcp_port>`

### Transfer Layer (TCP)
1. Receiver binds and listens on port **5000**
2. Sender connects using receiver's IP
3. File opened in binary mode
4. Data sent in **4KB buffer chunks**
5. Receiver writes chunks to output file
6. Connection closes after transfer

---

## 📊 Architecture

| Layer | Protocol | Port | Purpose |
|---|---|---|---|
| Discovery | UDP Broadcast | 5001 | Find peers on LAN |
| Transfer | TCP | 5000 | Reliable file transfer |

* **Model**: Decentralized P2P
* **I/O Model**: Blocking sockets
* **Platform**: Windows (Winsock2)

---

## 🔑 Key Concepts Used
* UDP Broadcast (`INADDR_BROADCAST`)
* TCP Socket Programming (`bind`, `listen`, `accept`, `connect`)
* File Handling (binary read/write)
* Buffer-based chunked transfer
* Peer list management (`std::map`)

---

## ⚠️ Notes
* Both devices must be on the **same LAN/WiFi**
* Firewall may block connections → allow ports **5000** and **5001**
* No encryption/authentication (basic version)

---

## 🚀 Roadmap

* [x] Phase 1 — TCP file transfer
* [x] Phase 2 — UDP peer discovery
* [ ] Phase 3 — Chunking + live progress bar
* [ ] Phase 4 — SHA-256 integrity + resume support
* [ ] End-to-end encryption (AES)
* [ ] GUI interface

---

## 🏆 Resume Highlight
> Built a decentralized P2P LAN file sharing system in C++ featuring UDP-based automatic peer discovery and TCP-based reliable file transfer, without any central server — inspired by AirDrop.

---

## 📌 Conclusion
This project demonstrates fundamental concepts of:
* Computer Networks
* Distributed Systems
* System Programming

Foundation for building:
* Torrent clients
* Cloud storage systems
* Peer-to-peer networks
