#pragma once
#include <cstdint>

const uint32_t CHUNK_SIZE_BYTES = 512 * 1024; // 512KB

// Sent once before chunks
struct FileMetadata {
    char     filename[256];
    uint64_t filesize;
    uint32_t total_chunks;
    uint32_t chunk_size;
};

// Sent before every chunk
struct ChunkHeader {
    uint32_t chunk_id;
    uint32_t chunk_size;
};

// Sent by receiver → tells sender which chunks it already has
struct ResumeRequest {
    uint32_t received_count;           // how many chunks already received
    uint32_t received_ids[10000];      // chunk IDs already received
};

// Sent by sender after all chunks → SHA-256 hex string
struct HashPacket {
    char hash[65];                     // 64 hex chars + null terminator
};