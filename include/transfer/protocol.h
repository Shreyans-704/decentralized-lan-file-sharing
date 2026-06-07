#pragma once
#include <cstdint>

// Sent once before chunks — tells receiver what's coming
struct FileMetadata {
    char     filename[256];   // original filename
    uint64_t filesize;        // total bytes
    uint32_t total_chunks;    // how many chunks to expect
    uint32_t chunk_size;      // bytes per chunk (last may be smaller)
};

// Header sent before every chunk
struct ChunkHeader {
    uint32_t chunk_id;        // 0-based index
    uint32_t chunk_size;      // actual bytes in this chunk
};