#!/usr/bin/env bash
set -euo pipefail

TARGET_DIR="${1:-pokeemerald}"

if [[ -d "$TARGET_DIR" && ! -d "$TARGET_DIR/.git" ]]; then
  echo "[error] $TARGET_DIR existe pero no parece un repo git."
  exit 1
fi

if [[ ! -d "$TARGET_DIR/.git" ]]; then
  echo "[info] Clonando pret/pokeemerald en $TARGET_DIR"
  git clone https://github.com/pret/pokeemerald "$TARGET_DIR"
fi

if [[ ! -d "$TARGET_DIR/include" || ! -d "$TARGET_DIR/src" ]]; then
  echo "[error] Repo destino no tiene estructura esperada (include/ y src/)."
  exit 1
fi

echo "[info] Copiando módulos roguelike base"
install -D -m 0644 integration/pokeemerald/include/roguelike.h "$TARGET_DIR/include/roguelike.h"
install -D -m 0644 integration/pokeemerald/include/roguelike_runtime.h "$TARGET_DIR/include/roguelike_runtime.h"
install -D -m 0644 integration/pokeemerald/src/roguelike.c "$TARGET_DIR/src/roguelike.c"
install -D -m 0644 integration/pokeemerald/src/roguelike_runtime.c "$TARGET_DIR/src/roguelike_runtime.c"

echo "[ok] Archivos copiados. En pret/pokeemerald no hace falta editar Makefile: src/*.c se detecta por wildcard."
