#!/usr/bin/env python3
"""Create an IPS patch from two ROM binaries.

Usage:
  python3 tools/make_ips_patch.py --base emerald_base.gba --target emerald_mod.gba --output roguelike.ips
"""

from __future__ import annotations

import argparse
from pathlib import Path

IPS_HEADER = b"PATCH"
IPS_EOF = b"EOF"
MAX_CHUNK = 0xFFFF


def _iter_diff_chunks(base: bytes, target: bytes):
    """Yield (offset, data_bytes) where target differs from base."""
    max_len = max(len(base), len(target))
    i = 0
    while i < max_len:
        b = base[i] if i < len(base) else None
        t = target[i] if i < len(target) else None
        if b == t:
            i += 1
            continue

        start = i
        chunk = bytearray()
        while i < max_len:
            b = base[i] if i < len(base) else None
            t = target[i] if i < len(target) else None
            if b == t:
                break
            chunk.append(t if t is not None else 0)
            i += 1

        yield start, bytes(chunk)


def _encode_ips(base: bytes, target: bytes) -> bytes:
    records = bytearray()

    for offset, data in _iter_diff_chunks(base, target):
        pos = 0
        while pos < len(data):
            part = data[pos : pos + MAX_CHUNK]
            current_offset = offset + pos
            if current_offset > 0xFFFFFF:
                raise ValueError("IPS only supports offsets up to 0xFFFFFF")
            records.extend(current_offset.to_bytes(3, "big"))
            records.extend(len(part).to_bytes(2, "big"))
            records.extend(part)
            pos += len(part)

    return IPS_HEADER + records + IPS_EOF


def create_patch(base_path: Path, target_path: Path, output_path: Path) -> None:
    base = base_path.read_bytes()
    target = target_path.read_bytes()

    patch = _encode_ips(base, target)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(patch)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create IPS patch from base and target ROM")
    parser.add_argument("--base", required=True, type=Path, help="Base ROM path (.gba)")
    parser.add_argument("--target", required=True, type=Path, help="Modified ROM path (.gba)")
    parser.add_argument("--output", required=True, type=Path, help="Output patch path (.ips)")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if not args.base.exists():
        raise FileNotFoundError(f"Base ROM not found: {args.base}")
    if not args.target.exists():
        raise FileNotFoundError(f"Target ROM not found: {args.target}")

    create_patch(args.base, args.target, args.output)
    print(f"IPS patch created: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
