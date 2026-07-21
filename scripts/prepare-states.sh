#!/usr/bin/env bash
# Seed the testnet's initial state files (spectrum.<EPOCH>, universe.<EPOCH>, ...)
# into the core-lite and bob volumes before first start.
#
# Source of the files, in order of preference:
#   1. ./states/  — provided by the dev (their own spectrum/universe/contract dumps)
#   2. ./state.zip — bundled EXAMPLE state (every computor seed funded), unzipped
#      into ./states with a warning
#
# core-lite loads spectrum.<EPOCH>/universe.<EPOCH> from its working dir at
# genesis; bob needs the same files to start verifying. Both get a copy.
# Idempotent: skips if the volumes are already seeded.
set -euo pipefail
cd "$(dirname "$0")/.."

PROJECT=$(docker compose config --format json | python3 -c 'import sys,json;print(json.load(sys.stdin)["name"])')
EPOCH=$(grep -oP '#define EPOCH \K[0-9]+' core-lite/src/public_settings.h)

# Generate bob's config from the template, pointing at core-lite's static IP.
# Bob resolves peers by raw IPv4 only, so this must match the compose network
# (.env: QUBIC_SUBNET / CORE_LITE_IP — override when 10.77.0.0/24 collides
# with an existing network on this host).
[ -f .env ] && . ./.env 2>/dev/null || true
CORE_LITE_IP="${CORE_LITE_IP:-10.77.0.10}"
sed "s/10\.77\.0\.10/${CORE_LITE_IP}/" docker/core-bob/bob.json > docker/core-bob/bob.gen.json
echo "== bob peer: BM:${CORE_LITE_IP}:31841 (docker/core-bob/bob.gen.json) =="

if docker run --rm -v "${PROJECT}_core_lite_data:/d" ubuntu:24.04 test -f "/d/spectrum.${EPOCH}" 2>/dev/null; then
  echo "volumes already seeded (spectrum.${EPOCH} in core_lite_data) — nothing to do"
  exit 0
fi

if [ ! -f "states/spectrum.${EPOCH}" ] || [ ! -f "states/universe.${EPOCH}" ]; then
  if [ ! -f state.zip ]; then
    echo "ERROR: no ./states/spectrum.${EPOCH}+universe.${EPOCH} and no ./state.zip fallback" >&2
    exit 1
  fi
  echo "!==========================================================================!"
  echo "! WARNING: ./states/ has no spectrum.${EPOCH}/universe.${EPOCH}."
  echo "! Falling back to the bundled EXAMPLE state (state.zip): an EMPTY genesis"
  echo "! — every balance is zero. Fund dev wallets with tools/spectrum_update"
  echo "! and tools/universe_update on ./states/ files, then rerun after a"
  echo "! full reset (docker compose down -v)."
  echo "!==========================================================================!"
  mkdir -p states
  unzip -o -q state.zip -d states
fi

echo "== seeding volumes from ./states =="
docker compose create core-lite core-bob >/dev/null 2>&1 || true   # ensure volumes exist
docker run --rm \
  -v "$(pwd)/states:/states:ro" \
  -v "${PROJECT}_core_lite_data:/core" \
  -v "${PROJECT}_bob_data:/bob" \
  -v "${PROJECT}_bob_redis:/r" \
  -v "${PROJECT}_bob_kvrocks:/k" \
  ubuntu:24.04 sh -c "cp /states/* /core/ && cp /states/* /bob/ && rm -rf /r/* /k/*"

echo "== done: $(ls states | tr '\n' ' ')=="
