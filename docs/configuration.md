# Configuration

Copy `.env.example` → `.env` and adjust (done automatically by `scripts/up.sh`).

| Variable | Default | Meaning |
|---|---|---|
| `TICK_DURATION_MS` | `1000` | ms per tick (runtime; higher = less disk) |
| `COMPOSE_PROFILES` | `explorer` | set empty to skip the whole explorer stack (core-lite + bob only, saves ~8 GB RAM) |
| `EPOCH_TICK_CAPACITY` | `5184000` | ticks per epoch before forced seamless rollover (**build arg**; 60 days at 1 s ticks) |
| `LITE_RAM` | `ON` | shrink node-local buffers (**build arg**, see below) |
| `PREFILL_QUS` | `OFF` | leave OFF: balances come from `./states` files, not minting (**build arg**) |
| `CORE_LITE_P2P_PORT` | `31841` | host port remap for core-lite P2P (sshd sometimes owns 31841) |
| `NGINX_PORT` | `80` | host port remap for the explorer UI |
| `QUBIC_SUBNET` | `10.77.0.0/24` | docker network subnet; change on "Pool overlaps" errors |
| `CORE_LITE_IP` | `10.77.0.10` | core-lite's static IP (must be inside `QUBIC_SUBNET`; bob's generated config points at it) |
| `ADMIN_API_KEY` | empty | enables explorer `/api/admin/*` (sent as `X-Api-Key`) |
| `PRUNER_DRY_RUN` | `true` | explorer pruner does not delete by default |

Build args (`EPOCH_TICK_CAPACITY`, `PREFILL_QUS`, `LITE_RAM`) need `docker compose build core-lite` to take effect.

## RAM usage

Measured on a 60 GB host (1 s ticks, defaults):

| Configuration | RAM |
|---|---|
| core-lite (`LITE_RAM=ON`, default) | ~16 GB |
| core-bob | ~5 GB |
| explorer stack (`COMPOSE_PROFILES=explorer`) | ~8 GB |
| **full stack** | **~29 GB** |
| lean (core-lite + bob, `COMPOSE_PROFILES=`) | ~21 GB |
| core-lite with `LITE_RAM=OFF` (stock buffers) | ~31 GB alone |

### What `LITE_RAM=ON` does

It enables core-lite's `TESTNET_LITE_RAM` build mode, which shrinks node-local buffers only — score cache, dejavu filter, logging/swap caches, oracle engine, miner flags, network queues. The Dockerfile additionally pins the spectrum/assets hash-map depth at 24 (stock `TESTNET_LITE_RAM` would shrink it to 18), so state files, digests, and bob's wire structs stay fully compatible: a LITE build boots from the same `./states` files and bob verifies identically.

The remaining ~16 GB floor is dominated by the compiled-in mainnet contract states (~6 GB) and the bob-compatible spectrum/universe tables (~4 GB).

LITE builds print a per-buffer `RAM <name> <MB>` ledger at startup:

```bash
docker compose logs core-lite | grep "RAM "
```

### Explorer on/off

The explorer services (ClickHouse, indexer, analytics, api, pruner, frontend, nginx) sit behind the `explorer` compose profile. `COMPOSE_PROFILES=explorer` (default) runs them; `COMPOSE_PROFILES=` (empty) skips them, saving ~8 GB RAM plus their disk usage. Toggle any time — the chain and bob's index are unaffected:

```bash
# turn off explorer services that are already running
docker compose --profile explorer stop
# manage/remove them explicitly when the profile is disabled in .env
docker compose --profile explorer down
```

Without the explorer you still have bob's JSON-RPC/REST/WebSocket on :40420 and core-lite's built-in explorer at http://localhost:41841/explorer.
