// Shared helpers for qubic state-file tools. Layouts mirror core-lite:
//   EntityRecord: core-lite/src/network_messages/entity.h (64 bytes)
//   AssetRecord:  core-lite/src/network_messages/assets.h (48 bytes)
//   identity decoding: core-lite/src/four_q.h getPublicKeyFromIdentity()
//   hash-map placement: pubkey.u32[0] & (capacity-1), linear probing
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct PubKey
{
    uint8_t b[32];
    bool operator==(const PubKey& o) const { return memcmp(b, o.b, 32) == 0; }
    bool isZero() const
    {
        for (int i = 0; i < 32; i++) if (b[i]) return false;
        return true;
    }
    uint32_t u32_0() const { uint32_t v; memcpy(&v, b, 4); return v; }
};

#pragma pack(push, 1)
struct EntityRecord
{
    PubKey publicKey;
    long long incomingAmount, outgoingAmount;
    unsigned int numberOfIncomingTransfers, numberOfOutgoingTransfers;
    unsigned int latestIncomingTransferTick, latestOutgoingTransferTick;
};
static_assert(sizeof(EntityRecord) == 64, "EntityRecord must be 64 bytes");

#define ASSET_EMPTY 0
#define ASSET_ISSUANCE 1
#define ASSET_OWNERSHIP 2
#define ASSET_POSSESSION 3

struct AssetRecord
{
    union
    {
        struct
        {
            PubKey publicKey;
            unsigned char type;
            char name[7];
            char numberOfDecimalPlaces;
            char unitOfMeasurement[7];
        } issuance;
        struct
        {
            PubKey publicKey;
            unsigned char type;
            char padding[1];
            unsigned short managingContractIndex;
            unsigned int issuanceIndex;
            long long numberOfShares;
        } ownership;
        struct
        {
            PubKey publicKey;
            unsigned char type;
            char padding[1];
            unsigned short managingContractIndex;
            unsigned int ownershipIndex;
            long long numberOfShares;
        } possession;
    } varStruct;
};
static_assert(sizeof(AssetRecord) == 48, "AssetRecord must be 48 bytes");
#pragma pack(pop)

// 60-char identity -> 32-byte public key (first 56 chars, base-26 little-endian
// per 8-byte fragment; the 4 checksum chars are not verified here).
inline bool identityToPubKey(const char* identity, PubKey& out)
{
    if (strlen(identity) != 60) return false;
    for (int i = 0; i < 4; i++)
    {
        uint64_t frag = 0;
        for (int j = 14; j-- > 0; )
        {
            char c = identity[i * 14 + j];
            if (c < 'A' || c > 'Z') return false;
            frag = frag * 26 + (c - 'A');
        }
        memcpy(out.b + i * 8, &frag, 8);
    }
    return true;
}

// Asset name: 1-7 chars, capital letters + digits. Fills name[8] zero-padded.
inline bool parseAssetName(const char* arg, char name[8])
{
    size_t len = strlen(arg);
    if (len == 0 || len > 7)
    {
        fprintf(stderr, "asset name must be 1-7 chars (capital letters + digits): %s\n", arg);
        return false;
    }
    for (size_t i = 0; i < len; i++)
    {
        char c = arg[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
        {
            fprintf(stderr, "asset name must be capital letters + digits: %s\n", arg);
            return false;
        }
    }
    memset(name, 0, 8);
    memcpy(name, arg, len);
    return true;
}

// Record-wise file IO: state files are 1.8 GB combined, never load them whole.
template <typename T>
bool readRec(FILE* f, uint64_t idx, T& rec)
{
    return fseeko(f, (off_t)idx * sizeof(T), SEEK_SET) == 0 && fread(&rec, sizeof(T), 1, f) == 1;
}

template <typename T>
bool writeRec(FILE* f, uint64_t idx, const T& rec)
{
    return fseeko(f, (off_t)idx * sizeof(T), SEEK_SET) == 0 && fwrite(&rec, sizeof(T), 1, f) == 1;
}

// Open state file and derive capacity (record count); must be a power of two.
template <typename T>
FILE* openStateFile(const char* path, uint64_t& capacity)
{
    FILE* f = fopen(path, "r+b");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return nullptr; }
    fseeko(f, 0, SEEK_END);
    off_t size = ftello(f);
    if (size <= 0 || size % sizeof(T) != 0)
    {
        fprintf(stderr, "%s: size %lld is not a multiple of %zu\n", path, (long long)size, sizeof(T));
        fclose(f);
        return nullptr;
    }
    capacity = (uint64_t)size / sizeof(T);
    if (capacity & (capacity - 1))
    {
        fprintf(stderr, "%s: capacity %llu is not a power of two\n", path, (unsigned long long)capacity);
        fclose(f);
        return nullptr;
    }
    return f;
}
