# Genesis state: funding wallets and issuing assets

Initial balances and assets come **only** from state files (`spectrum.<EPOCH>`, `universe.<EPOCH>`) loaded at genesis — the node mints nothing (`PREFILL_QUS=OFF`). `scripts/prepare-states.sh` (run by `up.sh`) seeds the files from `./states/` into the core-lite and bob volumes; if `./states/` is empty it unzips the bundled `state.zip` — an example genesis where **all 676 default computors hold 10'000'000'000 QU each** and everything else is empty — and prints a note. The script reads `EPOCH` from `core-lite/src/public_settings.h` and renames the unzipped files to `spectrum.<EPOCH>`/`universe.<EPOCH>` if the zip was made for a different epoch.

## Crafting state files

```bash
cd tools && make && cd ..                 # build the state-file tools once
mkdir -p states && unzip -o state.zip -d states

# credit 5B QU to a wallet (creates the entity if absent)
./tools/spectrum_update states/spectrum.222 <IDENTITY> 5000000000

# issue an asset: issuance record + issuer's initial supply
./tools/universe_issue  states/universe.222 <ISSUER_ID> MYCOIN 1000000

# grant shares of an issued asset to another wallet
./tools/universe_update states/universe.222 <OWNER_ID> MYCOIN <ISSUER_ID> 99999

./scripts/up.sh
```

## The tools

Built with `make` in `tools/`. All mirror core-lite's hash-map placement (linear probe from `pubkey.u32[0] & (capacity-1)`; ownership/possession records probe from the *owner's* hash slot, matching bob's lookups) and edit records in place with record-wise I/O.

| Tool | Usage | Behavior |
|---|---|---|
| `spectrum_update` | `spectrum_update spectrum.<EPOCH> <IDENTITY> <AMOUNT>` | adds QU to `incomingAmount`; creates the entity if absent |
| `universe_issue` | `universe_issue universe.<EPOCH> <ISSUER_ID> <NAME> <SHARES>` | creates issuance + issuer's ownership/possession; **errors if already issued** |
| `universe_update` | `universe_update universe.<EPOCH> <OWNER_ID> <NAME> <ISSUER_ID> <SHARES>` | grants shares to an owner; **errors if the asset was never issued** |

## Notes

- `<IDENTITY>` is a 60-char uppercase Qubic identity. The tools don't verify the 4 checksum chars, but bob's RPC does — query by `0x<hex public key>` once and use the canonical identity from the response.
- Asset names: 1–7 chars, `A-Z0-9`. Managing contract is QX (index 1).
- State file formats: spectrum = 2^24 × 64-byte entity records; universe = 2^24 × 48-byte asset records. Both sides (core-lite and bob) load the same files, so they must be identical.
- State files can only be changed **before genesis**. To apply new files: `./scripts/down.sh --wipe && ./scripts/up.sh`.
