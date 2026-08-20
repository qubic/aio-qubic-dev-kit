#!/usr/bin/env bash
# Stop the whole stack — all services, explorer included, regardless of
# COMPOSE_PROFILES.
#
#   ./scripts/down.sh          stop and remove containers, keep volumes
#   ./scripts/down.sh --wipe   also delete volumes (chain + bob index + ClickHouse)
#
# Note: core-lite cannot resume a stopped chain (the long-run testnet always
# restarts from genesis, and stale tick storage puts it in a relaunch loop),
# so before the next ./scripts/up.sh a --wipe is usually required anyway.
set -euo pipefail
cd "$(dirname "$0")/.."

if [ "${1:-}" = "--wipe" ]; then
  docker compose --profile explorer --profile oracle down -v
  echo "Stack stopped, volumes wiped. Next ./scripts/up.sh starts a fresh chain from genesis."
else
  docker compose --profile explorer --profile oracle down
  echo "Stack stopped, volumes kept. Note: resuming a chain is not supported —"
  echo "run './scripts/down.sh --wipe' before the next up.sh for a clean start."
fi
