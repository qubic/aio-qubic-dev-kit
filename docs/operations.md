# Day-to-day operation

```bash
./scripts/stack-stats.sh        # per-container RAM/CPU, volume disk usage, host headroom
docker compose logs -f core-bob | grep 'Current state'
./scripts/down.sh               # stop everything (explorer included), keep volumes
./scripts/down.sh --wipe        # stop everything and delete volumes (chain + index + ClickHouse)
./scripts/down.sh --wipe && ./scripts/up.sh   # full reset
```

`down.sh` always covers all services regardless of `COMPOSE_PROFILES`. A plain `down.sh` keeps the data, but core-lite cannot resume a stopped chain (the long-run testnet restarts from genesis, and stale tick storage puts the binary in a relaunch loop) — so use `--wipe` before the next `up.sh` unless you only stopped to free the machine temporarily and plan to wipe later anyway.

- Epochs never roll over by time; force one by sending **F7** to core-lite's stdin (`\x1b[18~`, one write).
- Never send a **lone ESC byte** to core-lite's stdin — it triggers shutdown.
- Remote box? Tunnel from your laptop: `ssh -L 80:localhost:80 -L 5000:localhost:5000 -L 40420:localhost:40420 user@host`.
- Pull the latest mainline of the three subprojects: `git submodule update --remote` (plain `git submodule update` returns to the pinned, tested commits).

## Exposing the testnet to the internet

Only two ports bind publicly (`0.0.0.0`): **80** (explorer UI — nginx proxies the API and websockets through it) and **40420** (bob JSON-RPC/REST/WS, including `qubic_broadcastTransaction`, so external users can send transactions). Everything else — ClickHouse 8123/9000, explorer API 5000, frontend 3000, core-lite 41841 and P2P 31841 — binds to `127.0.0.1` and is reachable from the host only (use an SSH tunnel: `ssh -L 41841:localhost:41841 user@host`).

To publish on a VPS, open **only TCP 80 and 40420** in the provider firewall / security group. Keep in mind the RPC has no authentication or rate limiting — anyone can query and broadcast — which is acceptable for a disposable test chain but treat the whole deployment as untrusted-public: fresh chain, no secrets, wipe when done.

## Service endpoints

| Service | URL |
|---|---|
| Explorer UI | http://localhost (nginx; `NGINX_PORT` remaps) |
| Explorer API | http://localhost:5000 (Swagger at `/swagger`) |
| Bob JSON-RPC | `POST http://localhost:40420/qubic`, `ws://localhost:40420/ws/qubic` |
| core-lite HTTP | http://localhost:41841 (`/live/v1`, `/query/v1`, `/explorer`) |
| ClickHouse | http://localhost:8123 |

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| bob stuck at `FetchingTick: <genesis>` forever | Missing `spectrum.<EPOCH>`/`universe.<EPOCH>` in its volume — run `./scripts/prepare-states.sh` (or full reset) |
| core-lite RSTs every new P2P connection | Testnet build has only **4 incoming slots**; a reconnect storm exhausts them. Stop the client, restart core-lite, start the client once |
| Explorer shows nothing | Check `docker compose logs indexer` — it needs bob reachable at `BOB_URL` (default `http://core-bob:40420`) |
| Bob rejects your identity | Checksum invalid — query by `0x<hex pubkey>` and use the canonical identity bob returns |
| Chain restarted after core-lite restart | Expected: `START_NETWORK_FROM_SCRATCH`. Bob's index must be reset too — full reset is the reliable path |
| core-lite in a relaunch loop ("Failed to load tick storage") | Stale tick-storage dirs after a restart — full reset |
| `Pool overlaps with other one on this address space` at network create | The default `10.77.0.0/24` collides with an existing docker network or VPN route — set `QUBIC_SUBNET` and `CORE_LITE_IP` in `.env`, then `./scripts/down.sh && ./scripts/up.sh` |
| Disk filling fast | Raise `TICK_DURATION_MS`; core-lite writes ~25–50 GB/day at 1 s ticks |
| RAM too high | `LITE_RAM=ON` (default) and disable the explorer — see [configuration.md](configuration.md) |
