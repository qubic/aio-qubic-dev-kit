// Increase an entity's balance in a spectrum state file (before genesis).
// Usage: spectrum_update spectrum.xxx ID_TO_UPDATE AMOUNT_TO_UPDATE
// Placement mirrors core-lite spectrum.h: index = pubkey.u32[0] & (cap-1), linear probe.
#include "state_common.h"

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s spectrum.xxx ID_TO_UPDATE AMOUNT_TO_UPDATE\n", argv[0]);
        return 1;
    }

    PubKey key;
    if (!identityToPubKey(argv[2], key) || key.isZero())
    {
        fprintf(stderr, "invalid identity: %s (need 60 uppercase chars)\n", argv[2]);
        return 1;
    }
    long long amount = strtoll(argv[3], nullptr, 10);
    if (amount <= 0)
    {
        fprintf(stderr, "amount must be a positive integer, got %s\n", argv[3]);
        return 1;
    }

    uint64_t cap;
    FILE* f = openStateFile<EntityRecord>(argv[1], cap);
    if (!f) return 1;

    uint64_t idx = key.u32_0() & (cap - 1);
    EntityRecord rec;
    for (uint64_t probes = 0; probes < cap; probes++, idx = (idx + 1) & (cap - 1))
    {
        if (!readRec(f, idx, rec)) { fprintf(stderr, "read failed at %llu\n", (unsigned long long)idx); fclose(f); return 1; }
        if (rec.publicKey == key)
        {
            rec.incomingAmount += amount;
            rec.numberOfIncomingTransfers++;
            if (!writeRec(f, idx, rec)) { fprintf(stderr, "write failed\n"); fclose(f); return 1; }
            printf("updated entity at index %llu: balance %lld -> %lld\n",
                   (unsigned long long)idx,
                   rec.incomingAmount - amount - rec.outgoingAmount,
                   rec.incomingAmount - rec.outgoingAmount);
            fclose(f);
            return 0;
        }
        if (rec.publicKey.isZero())
        {
            memset(&rec, 0, sizeof(rec));
            rec.publicKey = key;
            rec.incomingAmount = amount;
            rec.numberOfIncomingTransfers = 1;
            if (!writeRec(f, idx, rec)) { fprintf(stderr, "write failed\n"); fclose(f); return 1; }
            printf("created entity at index %llu: balance %lld\n", (unsigned long long)idx, amount);
            fclose(f);
            return 0;
        }
    }
    fprintf(stderr, "spectrum full, no slot found\n");
    fclose(f);
    return 1;
}
