// Issue an asset in a universe state file (before genesis): creates the
// issuance record plus the issuer's ownership and possession records holding
// the initial share supply — mirroring core-lite assets.h issueAsset().
// Fails if the asset is already issued. Run this before universe_update.
// Usage: universe_issue universe.xxx ISSUER_ID ASSET_NAME NUMBER_OF_SHARES
#include "state_common.h"

static const unsigned short MANAGING_CONTRACT_INDEX = 1; // QX

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "usage: %s universe.xxx ISSUER_ID ASSET_NAME NUMBER_OF_SHARES\n", argv[0]);
        return 1;
    }

    PubKey issuer;
    if (!identityToPubKey(argv[2], issuer) || issuer.isZero())
    {
        fprintf(stderr, "invalid issuer identity: %s\n", argv[2]);
        return 1;
    }

    char name[8] = {0};
    if (!parseAssetName(argv[3], name)) return 1;

    long long shares = strtoll(argv[4], nullptr, 10);
    if (shares <= 0)
    {
        fprintf(stderr, "number of shares must be a positive integer, got %s\n", argv[4]);
        return 1;
    }

    uint64_t cap;
    FILE* f = openStateFile<AssetRecord>(argv[1], cap);
    if (!f) return 1;

    AssetRecord rec;

    // issuance: find existing (error) or first empty slot from the issuer's hash
    uint64_t issuanceIdx = issuer.u32_0() & (cap - 1);
    long long emptyIdx = -1;
    for (uint64_t probes = 0; probes < cap; probes++, issuanceIdx = (issuanceIdx + 1) & (cap - 1))
    {
        if (!readRec(f, issuanceIdx, rec)) { fprintf(stderr, "read failed\n"); fclose(f); return 1; }
        if (rec.varStruct.issuance.type == ASSET_EMPTY) { emptyIdx = (long long)issuanceIdx; break; }
        if (rec.varStruct.issuance.type == ASSET_ISSUANCE
            && rec.varStruct.issuance.publicKey == issuer
            && memcmp(rec.varStruct.issuance.name, name, 7) == 0)
        {
            fprintf(stderr, "asset %s by this issuer already exists at index %llu\n",
                    argv[3], (unsigned long long)issuanceIdx);
            fclose(f);
            return 1;
        }
    }
    if (emptyIdx < 0) { fprintf(stderr, "universe full\n"); fclose(f); return 1; }
    issuanceIdx = (uint64_t)emptyIdx;

    memset(&rec, 0, sizeof(rec));
    rec.varStruct.issuance.publicKey = issuer;
    rec.varStruct.issuance.type = ASSET_ISSUANCE;
    memcpy(rec.varStruct.issuance.name, name, 7);
    if (!writeRec(f, issuanceIdx, rec)) { fprintf(stderr, "write failed\n"); fclose(f); return 1; }
    printf("created issuance %s at index %llu\n", argv[3], (unsigned long long)issuanceIdx);

    // issuer's ownership then possession, probing from the issuer's hash slot
    // (for the issuer this lands right after the issuance record, like issueAsset)
    uint64_t ownershipIdx = issuer.u32_0() & (cap - 1);
    for (uint64_t probes = 0; probes < cap; probes++, ownershipIdx = (ownershipIdx + 1) & (cap - 1))
    {
        if (!readRec(f, ownershipIdx, rec)) { fprintf(stderr, "read failed\n"); fclose(f); return 1; }
        if (rec.varStruct.ownership.type == ASSET_EMPTY) break;
    }
    memset(&rec, 0, sizeof(rec));
    rec.varStruct.ownership.publicKey = issuer;
    rec.varStruct.ownership.type = ASSET_OWNERSHIP;
    rec.varStruct.ownership.managingContractIndex = MANAGING_CONTRACT_INDEX;
    rec.varStruct.ownership.issuanceIndex = (unsigned int)issuanceIdx;
    rec.varStruct.ownership.numberOfShares = shares;
    if (!writeRec(f, ownershipIdx, rec)) { fprintf(stderr, "write failed\n"); fclose(f); return 1; }
    printf("created issuer ownership at index %llu: %lld shares\n", (unsigned long long)ownershipIdx, shares);

    uint64_t possessionIdx = issuer.u32_0() & (cap - 1);
    for (uint64_t probes = 0; probes < cap; probes++, possessionIdx = (possessionIdx + 1) & (cap - 1))
    {
        if (!readRec(f, possessionIdx, rec)) { fprintf(stderr, "read failed\n"); fclose(f); return 1; }
        if (rec.varStruct.possession.type == ASSET_EMPTY) break;
    }
    memset(&rec, 0, sizeof(rec));
    rec.varStruct.possession.publicKey = issuer;
    rec.varStruct.possession.type = ASSET_POSSESSION;
    rec.varStruct.possession.managingContractIndex = MANAGING_CONTRACT_INDEX;
    rec.varStruct.possession.ownershipIndex = (unsigned int)ownershipIdx;
    rec.varStruct.possession.numberOfShares = shares;
    if (!writeRec(f, possessionIdx, rec)) { fprintf(stderr, "write failed\n"); fclose(f); return 1; }
    printf("created issuer possession at index %llu: %lld shares\n", (unsigned long long)possessionIdx, shares);

    fclose(f);
    return 0;
}
