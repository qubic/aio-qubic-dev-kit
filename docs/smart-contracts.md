# Developing a smart contract

Qubic contracts are C++ compiled into the node. Iteration loop:

## 1. Write it

```bash
cp core-lite/src/contracts/EmptyTemplate.h core-lite/src/contracts/MyContract.h
```

- Rename the `CNAME`/`CNAME2` structs, keep persistent state inside `StateData`.
- Implement user functions (reads) and user procedures (transactions) plus the system hooks you need (`INITIALIZE`, `BEGIN_TICK`, `END_EPOCH`, …).
- Only the QPI (`core-lite/src/contracts/qpi.h`) is available — no stdlib.
- Register the contract in `core-lite/src/contract_core/contract_def.h` with the next unused index (copy the pattern of an existing entry).
- Read `core-lite/doc/contracts.md` and `core-lite/doc/execution_fees.md` first: state size and every `state.mut()` cost fees; keep state small.

## 2. Rebuild and restart the chain

Contracts are compile-time ⇒ a contract change means a fresh chain:

```bash
docker compose build core-lite
./scripts/down.sh --wipe && ./scripts/up.sh
```

## 3. Fund a dev wallet

See [genesis-state.md](genesis-state.md) — credit your identity in `states/spectrum.222` before `up.sh`.

## 4. Exercise it

- Send transactions (invoke procedures) via bob: `qubic_broadcastTransaction` on `POST :40420/qubic`, or with `qubic-cli` (buildable from `core-lite`, target `qubic-cli`) against `localhost:31841`.
- Call view functions: `POST http://localhost:41841/query/v1` on core-lite, or bob's RPC.
- Watch everything in the explorer: transactions, per-tick logs, balance changes. Contract log messages (`LOG_CONTRACT_*`) flow through bob — this build has logging events fully enabled.

## 5. Unit tests

Faster than full-stack runs: configure core-lite with `-D BUILD_TESTS:BOOL=ON` (GoogleTest, see `core-lite/test/` for contract test examples).
