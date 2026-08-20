#!/usr/bin/env bash
# One-command testnet: seed state files, build and start the whole stack.
#
#   ./scripts/up.sh             core-lite + bob + explorer (per .env COMPOSE_PROFILES)
#   ./scripts/up.sh --oracle    additionally build & run the oracle machine
#                               (adds "oracle" profile, defaults ORACLE_MACHINE_IP,
#                               rebuilds core-lite with the OM IP compiled in)
set -euo pipefail
cd "$(dirname "$0")/.."

ORACLE=0
for arg in "$@"; do
  case "$arg" in
    --oracle) ORACLE=1 ;;
    *) echo "Unknown option: $arg (supported: --oracle)"; exit 1 ;;
  esac
done

[ -f .env ] || cp .env.example .env

if [ "$ORACLE" = 1 ]; then
  set -a; . ./.env; set +a
  export ORACLE_MACHINE_IP="${ORACLE_MACHINE_IP:-10.77.0.20}"
  case ",${COMPOSE_PROFILES:-}," in
    *,oracle,*) ;;
    *) export COMPOSE_PROFILES="${COMPOSE_PROFILES:+${COMPOSE_PROFILES},}oracle" ;;
  esac
fi

docker compose build
./scripts/prepare-states.sh
docker compose up -d

echo
echo "Testnet up. Explorer: http://localhost:\${NGINX_PORT:-80} | Bob RPC: :40420 | core-lite HTTP: :41841"
if [ "$ORACLE" = 1 ]; then
  echo "Oracle machine on (OM node at ${ORACLE_MACHINE_IP}:31841). Logs: docker compose logs -f oracle-machine"
fi
echo "Watch sync: docker compose logs -f core-bob | grep 'Current state'"
