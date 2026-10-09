#!/usr/bin/env bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

LOG_FILE="${PROJECT_ROOT}/build_last_error.log"
rm -f "$LOG_FILE"

echo "[+] Starting full SDK build inside container..."
docker run --rm -v "${PROJECT_ROOT}:/workspace" -w /workspace/references/wifi6-sdk openwrt-mtk-builder \
  bash -c "make -j\$(nproc) || make V=s -j1" 2>&1 | tee "$LOG_FILE"

if [ ${PIPESTATUS[0]} -ne 0 ]; then
  echo "[-] Build FAILED! Check build_last_error.log"
  exit 1
else
  echo "[+] Build SUCCESS! Images are in references/wifi6-sdk/bin/targets/"
fi