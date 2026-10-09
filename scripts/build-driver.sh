#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Docker imajı yoksa build et
if [[ "$(docker images -q openwrt-mtk-builder 2> /dev/null)" == "" ]]; then
  echo "[+] Building Docker image openwrt-mtk-builder..."
  docker build -t openwrt-mtk-builder -f "${PROJECT_ROOT}/docker/Dockerfile" "${PROJECT_ROOT}/docker"
fi

echo "[+] Compiling mt76/mt7915 driver inside container..."
docker run --rm -v "${PROJECT_ROOT}:/workspace" -w /workspace/references/wifi6-sdk openwrt-mtk-builder \
  bash -c "make package/kernel/mt76/compile V=s -j\$(nproc)"