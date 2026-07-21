#!/usr/bin/env bash
# One-command testnet: seed state files, build and start the whole stack.
set -euo pipefail
cd "$(dirname "$0")/.."

[ -f .env ] || cp .env.example .env
docker compose build
./scripts/prepare-states.sh
docker compose up -d

echo
echo "Testnet up. Explorer: http://localhost:\${NGINX_PORT:-80} | Bob RPC: :40420 | core-lite HTTP: :41841"
echo "Watch sync: docker compose logs -f core-bob | grep 'Current state'"
