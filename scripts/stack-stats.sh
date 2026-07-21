#!/usr/bin/env bash
# Resource snapshot for the aio stack: per-container RAM/CPU, volume disk usage, host headroom.
set -euo pipefail
cd "$(dirname "$0")/.."

PROJECT=$(docker compose config --format json | python3 -c 'import sys,json;print(json.load(sys.stdin)["name"])')

echo "== containers (RAM / CPU) =="
docker stats --no-stream --format 'table {{.Name}}\t{{.MemUsage}}\t{{.MemPerc}}\t{{.CPUPerc}}' \
  | { read -r h; echo "$h"; grep "$PROJECT"; }

echo
echo "== volumes (disk) =="
for v in core_lite_data bob_redis bob_kvrocks bob_data clickhouse_data; do
  size=$(docker run --rm -v "${PROJECT}_${v}:/d" ubuntu:24.04 du -sh /d 2>/dev/null | cut -f1)
  printf '%-20s %s\n' "$v" "$size"
done

echo
echo "== host =="
free -h | head -2
df -h "$(docker info --format '{{.DockerRootDir}}' 2>/dev/null || echo /var/lib/docker)" | tail -1
