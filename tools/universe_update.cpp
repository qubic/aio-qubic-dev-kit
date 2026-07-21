// Grant asset shares to an entity in a universe state file (before genesis).
// Updates (or creates) the issuance record, then the owner's ownership and
// possession records, matching core-lite assets.h placement (issueAsset /
// transferShareOwnershipAndPossession) and bob's find*Index() lookups:
//   issuance:   probe from pubkey(issuer).u32[0] & (cap-1)
//   ownership:  probe from pubkey(owner).u32[0] & (cap-1)
//   possession: probe from pubkey(owner).u32[0] & (cap-1)
// Usage: universe_update universe.xxx ID_TO_UPDATE ASSET_NAME ASSET_ISSUER AMOUNT_TO_UPDATE
#include "state_common.h"

static const unsigned short MANAGING_CONTRACT_INDEX = 1; // QX

int main(int argc, char** argv)
{
    if (argc != 6)
    {
        fprintf(stderr, "usage: %s universe.xxx ID_TO_UPDATE ASSET_NAME ASSET_ISSUER AMOUNT_TO_UPDATE\n", argv[0]);
        return 1;
    }

    PubKey owner, issuer;
    if (!identityToPubKey(argv[2], owner) || owner.isZero())
    {
        fprintf(stderr, "invalid owner identity: %s\n", argv[2]);
        return 1;
    }
    if (!identityToPubKey(argv[4], issuer) || issuer.isZero())
    {
        fprintf(stderr, "invalid issuer identity: %s\n", argv[4]);
        return 1;
    }

    char name[8] = {0};
    if (!parseAssetName(argv[3], name)) return 1;

    long long amount = strtoll(argv[5], nullptr, 10);
    if (amount <= 0)
    {
        fprintf(stderr, "amount must be a positive integer, got %s\n", argv[5]);
        return 1;
    }

    uint64_t cap;
    FILE* f = openStateFile<AssetRecord>(argv[1], cap);
    if (!f) return 1;

    AssetRecord rec;

    // 1. issuance record must already exist (create it with universe_issue first)
    uint64_t issuanceIdx = issuer.u32_0() & (cap - 1);
    bool found = false;
    for (uint64_t probes = 0; probes < cap; probes++, issuanceIdx = (issuanceIdx + 1) & (cap - 1))
    {
        if (!readRec(f, issuanceIdx, rec)) { fprintf(stderr, "read failed\n"); fclose(f); return 1; }
        if (rec.varStruct.issuance.type == ASSET_EMPTY) break;
        if (rec.varStruct.issuance.type == ASSET_ISSUANCE
            && rec.varStruct.issuance.publicKey == issuer
            && memcmp(rec.varStruct.issuance.name, name, 7) == 0)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        fprintf(stderr, "asset %s by issuer %s does not exist — issue it first with universe_issue\n",
                argv[3], argv[4]);
        fclose(f);
        return 1;
    }

    // 2. ownership record for the owner: find or create, probing from the owner's hash slot
    uint64_t ownershipIdx = owner.u32_0() & (cap - 1);
    found = false;
    for (uint64_t probes = 0; probes < cap; probes++, ownershipIdx = (ownershipIdx + 1) & (cap - 1))
    {
        if (!readRec(f, ownershipIdx, rec)) { fprintf(stderr, "read failed\n"); fclose(f); return 1; }
        if (rec.varStruct.ownership.type == ASSET_EMPTY)
        {
            memset(&rec, 0, sizeof(rec));
            rec.varStruct.ownership.publicKey = owner;
            rec.varStruct.ownership.type = ASSET_OWNERSHIP;
            rec.varStruct.ownership.managingContractIndex = MANAGING_CONTRACT_INDEX;
            rec.varStruct.ownership.issuanceIndex = (unsigned int)issuanceIdx;
            rec.varStruct.ownership.numberOfShares = amount;
            if (!writeRec(f, ownershipIdx, rec)) { fprintf(stderr, "write failed\n"); fclose(f); return 1; }
            printf("created ownership at index %llu: %lld shares\n", (unsigned long long)ownershipIdx, amount);
            found = true;
            break;
        }
        if (rec.varStruct.ownership.type == ASSET_OWNERSHIP
            && rec.varStruct.ownership.publicKey == owner
            && rec.varStruct.ownership.issuanceIndex == (unsigned int)issuanceIdx)
        {
            rec.varStruct.ownership.numberOfShares += amount;
            if (!writeRec(f, ownershipIdx, rec)) { fprintf(stderr, "write failed\n"); fclose(f); return 1; }
            printf("updated ownership at index %llu: %lld shares\n",
                   (unsigned long long)ownershipIdx, rec.varStruct.ownership.numberOfShares);
            found = true;
            break;
        }
    }
    if (!found) { fprintf(stderr, "universe full\n"); fclose(f); return 1; }

    // 3. possession record: find or create, probing from the owner's hash slot
    uint64_t possessionIdx = owner.u32_0() & (cap - 1);
    for (uint64_t probes = 0; probes < cap; probes++, possessionIdx = (possessionIdx + 1) & (cap - 1))
    {
        if (!readRec(f, possessionIdx, rec)) { fprintf(stderr, "read failed\n"); fclose(f); return 1; }
        if (rec.varStruct.possession.type == ASSET_EMPTY)
        {
            memset(&rec, 0, sizeof(rec));
            rec.varStruct.possession.publicKey = owner;
            rec.varStruct.possession.type = ASSET_POSSESSION;
            rec.varStruct.possession.managingContractIndex = MANAGING_CONTRACT_INDEX;
            rec.varStruct.possession.ownershipIndex = (unsigned int)ownershipIdx;
            rec.varStruct.possession.numberOfShares = amount;
            if (!writeRec(f, possessionIdx, rec)) { fprintf(stderr, "write failed\n"); fclose(f); return 1; }
            printf("created possession at index %llu: %lld shares\n", (unsigned long long)possessionIdx, amount);
            fclose(f);
            return 0;
        }
        if (rec.varStruct.possession.type == ASSET_POSSESSION
            && rec.varStruct.possession.publicKey == owner
            && rec.varStruct.possession.ownershipIndex == (unsigned int)ownershipIdx)
        {
            rec.varStruct.possession.numberOfShares += amount;
            if (!writeRec(f, possessionIdx, rec)) { fprintf(stderr, "write failed\n"); fclose(f); return 1; }
            printf("updated possession at index %llu: %lld shares\n",
                   (unsigned long long)possessionIdx, rec.varStruct.possession.numberOfShares);
            fclose(f);
            return 0;
        }
    }
    fprintf(stderr, "universe full\n");
    fclose(f);
    return 1;
}
