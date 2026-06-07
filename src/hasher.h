#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cstring>

// ── Minimal SHA-256 implementation ────────────────────────────────────
class SHA256 {
public:
    SHA256() { reset(); }

    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; i++) {
            m_data[m_blocklen++] = data[i];
            if (m_blocklen == 64) {
                transform();
                m_bitlen += 512;
                m_blocklen = 0;
            }
        }
    }

    std::string digest() {
        uint8_t hash[32];
        pad();
        revert(hash);
        std::ostringstream oss;
        for (auto b : hash)
            oss << std::hex << std::setw(2)
                << std::setfill('0') << (int)b;
        return oss.str();
    }

    static std::string hashFile(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return "";
        SHA256 sha;
        char buf[8192];
        while (f.read(buf, sizeof(buf)) || f.gcount() > 0)
            sha.update((uint8_t*)buf, f.gcount());
        return sha.digest();
    }

    static std::string hashBytes(const char* data, size_t len) {
        SHA256 sha;
        sha.update((uint8_t*)data, len);
        return sha.digest();
    }

private:
    uint8_t  m_data[64]{};
    uint32_t m_blocklen = 0;
    uint64_t m_bitlen   = 0;
    uint32_t m_state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    static const uint32_t K[64];

    static uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }

    void transform() {
        uint32_t w[64];
        for (int i = 0; i < 16; i++)
            w[i] = (m_data[i*4]<<24)|(m_data[i*4+1]<<16)|
                   (m_data[i*4+2]<<8)|m_data[i*4+3];
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3);
            uint32_t s1 = rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        uint32_t a=m_state[0], b=m_state[1], c=m_state[2], d=m_state[3];
        uint32_t e=m_state[4], f=m_state[5], g=m_state[6], h=m_state[7];
        for (int i = 0; i < 64; i++) {
            uint32_t S1  = rotr(e,6)^rotr(e,11)^rotr(e,25);
            uint32_t ch  = (e&f)^(~e&g);
            uint32_t t1  = h+S1+ch+K[i]+w[i];
            uint32_t S0  = rotr(a,2)^rotr(a,13)^rotr(a,22);
            uint32_t maj = (a&b)^(a&c)^(b&c);
            uint32_t t2  = S0+maj;
            h=g; g=f; f=e; e=d+t1;
            d=c; c=b; b=a; a=t1+t2;
        }
        m_state[0]+=a; m_state[1]+=b; m_state[2]+=c; m_state[3]+=d;
        m_state[4]+=e; m_state[5]+=f; m_state[6]+=g; m_state[7]+=h;
    }

    void pad() {
        uint64_t i = m_blocklen;
        m_data[i++] = 0x80;
        if (m_blocklen < 56) {
            while (i < 56) m_data[i++] = 0x00;
        } else {
            while (i < 64) m_data[i++] = 0x00;
            transform();
            memset(m_data, 0, 56);
        }
        m_bitlen += m_blocklen * 8;
        for (int j = 7; j >= 0; j--) {
            m_data[56+7-j] = (m_bitlen >> (j*8)) & 0xff;
        }
        transform();
    }

    void revert(uint8_t* hash) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 8; j++)
                hash[i+j*4] = (m_state[j] >> (24-i*8)) & 0xff;
    }

    void reset() {
        m_blocklen = 0; m_bitlen = 0;
        m_state[0]=0x6a09e667; m_state[1]=0xbb67ae85;
        m_state[2]=0x3c6ef372; m_state[3]=0xa54ff53a;
        m_state[4]=0x510e527f; m_state[5]=0x9b05688c;
        m_state[6]=0x1f83d9ab; m_state[7]=0x5be0cd19;
    }
};

