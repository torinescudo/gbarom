#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 ]]; then
  echo "Usage: $0 <pokeemerald_repo_path> <base_rom.gba> <output_patch.ips>"
  exit 1
fi

REPO_PATH="$1"
BASE_ROM="$2"
OUTPUT_PATCH="$3"

if [[ ! -d "$REPO_PATH" ]]; then
  echo "Repo path not found: $REPO_PATH"
  exit 1
fi

if [[ ! -f "$BASE_ROM" ]]; then
  echo "Base ROM not found: $BASE_ROM"
  exit 1
fi

if [[ ! -f "$REPO_PATH/Makefile" ]]; then
  echo "Not a pokeemerald repo (missing Makefile): $REPO_PATH"
  exit 1
fi

# Build modded ROM from source
make -C "$REPO_PATH" -j"$(nproc)"

TARGET_ROM="$REPO_PATH/pokeemerald.gba"
if [[ ! -f "$TARGET_ROM" ]]; then
  echo "Build finished but ROM not found: $TARGET_ROM"
  exit 1
fi

python3 "$(dirname "$0")/../tools/make_ips_patch.py" \
  --base "$BASE_ROM" \
  --target "$TARGET_ROM" \
  --output "$OUTPUT_PATCH"

echo "Done: $OUTPUT_PATCH"
