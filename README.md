# aio-qubic-dev-kit — all-in-one local Qubic testnet

One `docker compose` stack that wires three Qubic projects into a private, self-contained dev network:

```
core-lite ──P2P :31841──▶ core-bob ──RPC/WS :40420──▶ explorer indexer ──▶ ClickHouse
(C++ node, 676 computors,   (C++ indexer,                    │
 long-run local testnet)     KeyDB + Kvrocks)                ▼
                                              explorer API :5000 ◀── frontend/nginx :80
```

The network is sealed: no mainnet peers, no discovery, no phone-home.

## Prerequisites

- Linux x86-64 host with **AVX2**, Ubuntu 24.04 recommended
- Disk with room to grow: core-lite writes **~25–50 GB/day** at the default 1 s tick rate
- RAM: see the two setups below

One-liner for Ubuntu 24.04 (Docker + compose v2 + host tools):

```bash
sudo apt-get update && sudo apt-get install -y docker.io docker-compose-v2 git python3 make g++ unzip
```

(Optionally `sudo usermod -aG docker $USER` and re-login to run docker without sudo.)

## Run it — big machine (full stack, with explorer)

**Needs ≥32 GB RAM** (measured: ~29 GB — core-lite ~16, bob ~5, explorer ~8) and 8+ cores.

```bash
git clone --recurse-submodules https://github.com/qubic/aio-qubic-dev-kit.git && cd aio-qubic-dev-kit
./scripts/up.sh
```

Explorer UI at http://localhost, explorer API at :5000, bob JSON-RPC at :40420, core-lite HTTP at :41841. Watch the chain: `docker compose logs -f core-bob | grep 'Current state'` — healthy when FetchingTick advances ~1 tick/s.

## Run it — small machine (no explorer)

**The explorer stack (ClickHouse + .NET indexer/API/analytics/pruner + frontend + nginx) costs ~8 GB RAM — weak machines should not run it.** Without it the stack needs **≥24 GB RAM** (~21 GB measured), and you still have full RPC access through bob plus core-lite's built-in explorer at http://localhost:41841/explorer.

```bash
git clone --recurse-submodules https://github.com/qubic/aio-qubic-dev-kit.git && cd aio-qubic-dev-kit
cp .env.example .env
sed -i 's/^COMPOSE_PROFILES=.*/COMPOSE_PROFILES=/' .env   # empty = explorer OFF
./scripts/up.sh
```

Turning the explorer on/off later:

```bash
# ON:  set COMPOSE_PROFILES=explorer in .env, then
docker compose up -d
# OFF: set COMPOSE_PROFILES= (empty) in .env, then
docker compose --profile explorer stop && docker compose up -d
```

The chain and bob's index are untouched by toggling — only the explorer services start/stop. (`~16 GB` of the total is core-lite itself; see [docs/configuration.md](docs/configuration.md) for what `LITE_RAM` shrinks and the full RAM breakdown.)

## Optional: oracle machine

The [oracle-machine](oracle-machine/) middleware (OM node + price/mock/doge oracle services) is **off by default**. Enable it explicitly:

```bash
./scripts/up.sh --oracle
```

This adds the `oracle` compose profile and rebuilds core-lite with the OM node's IP (`ORACLE_MACHINE_IP`, default `10.77.0.20`) compiled in — so toggling it restarts the chain. Optional exchange API keys for the price service go in `.env` (`BINANCE_API_KEY`, `MEXC_API_KEY`, `GATE_API_KEY`). Details in [docs/configuration.md](docs/configuration.md).

## Custom settings example

All knobs live in `.env` ([docs/configuration.md](docs/configuration.md) has the full list). Example — slower ticks (halves disk usage) and a remapped P2P host port (e.g. when 31841 is taken by sshd):

```bash
cp -n .env.example .env
sed -i 's/^TICK_DURATION_MS=.*/TICK_DURATION_MS=2000/; s/^CORE_LITE_P2P_PORT=.*/CORE_LITE_P2P_PORT=50000/' .env
./scripts/up.sh
```

Changing `TICK_DURATION_MS` on an already-running deployment recreates core-lite and restarts the chain — do `./scripts/down.sh --wipe && ./scripts/up.sh` instead of editing in place. `CORE_LITE_P2P_PORT` only remaps the host side; bob's internal connection is unaffected.

## Documentation

| Doc | Contents |
|---|---|
| [docs/configuration.md](docs/configuration.md) | `.env` knobs, build args, RAM usage breakdown, port remaps |
| [docs/genesis-state.md](docs/genesis-state.md) | funding wallets, issuing assets, the `tools/` state-file editors |
| [docs/smart-contracts.md](docs/smart-contracts.md) | developing and testing a smart contract on this testnet |
| [docs/operations.md](docs/operations.md) | day-to-day commands, resets, epoch control, troubleshooting |
