# 🚀 Decentralized LAN File Sharing

A C++ peer-to-peer file transfer system for local networks — inspired by AirDrop. Features automatic peer discovery, chunked transfer, real-time progress, SHA-256 integrity verification, and resume support.

---

## 📌 Overview

This project implements a **decentralized file sharing system** where devices discover each other automatically and communicate directly over LAN without any central server.

It demonstrates:
- Low-level **socket programming** (TCP + UDP)
- **UDP-based automatic peer discovery**
- **TCP-based reliable chunked file transfer**
- **SHA-256 integrity verification**
- **Resume support** for interrupted transfers
- Real-world distributed systems concepts

---

## 🏗️ Project Structure

```
decentralized-lan-file-sharing/
│
├── src/
│   ├── sender.cpp              # TCP sender (chunked + progress + SHA-256)
│   ├── receiver.cpp            # TCP receiver (chunked + verify + resume)
│   ├── hasher.h                # Header-only SHA-256 implementation
│   └── discovery/
│       ├── broadcaster.cpp     # UDP broadcaster
│       └── listener.cpp        # UDP listener + peer list
│
├── include/
│   ├── transfer/
│   │   └── protocol.h          # Packet structs
│   └── discovery/
│       └── peer.h              # Peer struct
│
├── CMakeLists.txt
└── README.md
```

---

## 🧠 System Architecture

![Architecture](https://github.com/user-attachments/assets/e5f8a44a-bd1a-4df0-9409-5764430a2ea1)

---

## ⚙️ Building the Project

### Prerequisites
- C++17 compiler (MinGW / GCC)
- CMake 3.10+
- Windows (Winsock2)

### Build Steps

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

Produces 4 executables:

```
build/sender.exe
build/receiver.exe
build/broadcaster.exe
build/listener.exe
```

---

## ▶️ Usage

### 📡 Step 1 — Peer Discovery (both machines)

**Terminal 1 — Listener:**
```bash
build\listener.exe
```
```
[*] Listener started on port 5001
[*] Waiting for peers...

[+] New peer found: Shreyans-PC 

========= Available Peers =========
  1. Shreyans-PC 
===================================
```

**Terminal 2 — Broadcaster:**
```bash
build\broadcaster.exe
```
```
[*] Broadcasting as: Shreyans-PC
[*] Broadcasting every 2 seconds on port 5001
[+] Broadcast sent: DISCOVER|Shreyans-PC|5000
```

---

### 📁 Step 2 — File Transfer

**Terminal 1 — Receiver (start first):**
```bash
build\receiver.exe
```

**Terminal 2 — Sender:**
```bash
build\sender.exe <receiver_ip> <file_path>
```

Example:
```bash
build\sender.exe 192.168.1.8 movie.mp4
```

---

## 🧪 Full Example Output

### Sender:
```
[*] Computing SHA-256...
[+] Hash: e5b844cc57f57094ea4585e235f36c78c1cd222262bb89d53c94dcb4d6b3e55d
[*] File     : testfile.bin
[*] Size     : 10485760 bytes
[*] Chunks   : 20 x 512 KB
[*] Connecting to 127.0.0.1...
[+] Connected!

[##################################################] 100% | 88.43 MB/s

[+] File sent!
[+] SHA-256 sent: e5b844cc57f57094ea4585e235f36c78c1cd222262bb89d53c94dcb4d6b3e55d
```

### Receiver:
```
[*] Receiver listening on port 5000
[*] Waiting for sender...
[+] Connected from: 127.0.0.1
[*] Filename     : testfile.bin
[*] File size    : 10485760 bytes
[*] Total chunks : 20

[*] Receiving... 100% (10485760/10485760 bytes)

[*] Verifying integrity...
[*] Sender hash  : e5b844cc57f57094ea4585e235f36c78c1cd222262bb89d53c94dcb4d6b3e55d
[*] Received hash: e5b844cc57f57094ea4585e235f36c78c1cd222262bb89d53c94dcb4d6b3e55d

[+] SUCCESS: Integrity verified - file is perfect!
[+] File saved as : received_testfile.bin
[+] Total received: 10485760 bytes
```

---

## 🔄 How It Works

```
Both devices run broadcaster + listener
              ↓
UDP broadcast → "DISCOVER|hostname|5000"
              ↓
Listener builds live peer list
              ↓
User picks target device IP
              ↓
TCP connection established (port 5000)
              ↓
Receiver sends resume state → sender skips already received chunks
              ↓
FileMetadata sent → filename, filesize, total chunks
              ↓
File sent as 512KB chunks with ChunkHeader per chunk
              ↓
Receiver writes each chunk at correct file offset
              ↓
Sender sends SHA-256 hash
              ↓
Receiver verifies hash ✅
```

---

## 📦 Protocol Design

| Packet | Fields | Direction |
|---|---|---|
| `ResumeRequest` | received_count, chunk_ids[] | Receiver → Sender |
| `FileMetadata` | filename, filesize, total_chunks, chunk_size | Sender → Receiver |
| `ChunkHeader` | chunk_id, chunk_size | Sender → Receiver |
| `HashPacket` | hash[65] | Sender → Receiver |

```
Receiver              Sender
   |←── ResumeRequest ───|   "I have chunks 0–11"
   |──── FileMetadata ──→|   "Sending movie.mp4, 400 chunks"
   |──── ChunkHeader ───→|   "Chunk #12, 524288 bytes"
   |──── Chunk Data ────→|
   |──── ChunkHeader ───→|   "Chunk #13..."
   |──── Chunk Data ────→|
   |         ...          |
   |──── HashPacket ────→|   "SHA-256: e5b844..."
   |    [verify locally]  |
```

---

## 📊 Architecture Overview

| Layer | Protocol | Port | Purpose |
|---|---|---|---|
| Discovery | UDP Broadcast | 5001 | Auto peer detection |
| Transfer | TCP | 5000 | Reliable file transfer |

| Property | Value |
|---|---|
| Chunk size | 512 KB |
| Hash algorithm | SHA-256 (header-only) |
| Platform | Windows / Winsock2 |
| Language | C++17 |

---

## 🔑 Key Concepts Used

- UDP Broadcast (`INADDR_BROADCAST`, `SO_BROADCAST`)
- TCP Socket Programming (`bind`, `listen`, `accept`, `connect`)
- Binary protocol with custom packet headers
- Chunked file I/O with `seekp` for offset writing
- SHA-256 hashing (no external dependencies)
- Resume support via `chunks.log`
- Peer list via `std::map<IP, Peer>`

---

## ⚠️ Notes

- Both devices must be on the **same LAN/WiFi**
- Allow ports **5000** (TCP) and **5001** (UDP) through firewall
- Resume is automatic — just restart both executables
- Output saved as `received_<filename>` in working directory

---

## 🚀 Roadmap

- [x] Phase 1 — TCP file transfer
- [x] Phase 2 — UDP peer discovery
- [x] Phase 3 — Chunking + live progress bar
- [x] Phase 4 — SHA-256 integrity + resume support
- [ ] Unified single application
- [ ] AES-256 encryption
- [ ] Multi-peer parallel transfer
- [ ] GUI (Dear ImGui)

---

## 🏆 Resume Highlight

> Built a decentralized P2P LAN file sharing system in C++ with UDP-based automatic peer discovery, TCP-based chunked file transfer at **88+ MB/s**, SHA-256 integrity verification, and fault-tolerant resume support — without any central server, inspired by AirDrop.

---

## 📌 Conclusion

Demonstrates core concepts of:
- **Computer Networks** — TCP/UDP, sockets, broadcast
- **Distributed Systems** — decentralized, no master node
- **System Programming** — binary protocols, file I/O, C++17

Foundation for building BitTorrent clients, cloud storage systems, and peer-to-peer applications.
