import importlib.util
import sys
from pathlib import Path

SPEC = importlib.util.spec_from_file_location("make_ips_patch", Path("tools/make_ips_patch.py"))
make_ips_patch = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
sys.modules[SPEC.name] = make_ips_patch
SPEC.loader.exec_module(make_ips_patch)


def _apply_ips(base: bytes, patch: bytes) -> bytes:
    assert patch.startswith(b"PATCH")
    i = 5
    out = bytearray(base)

    while patch[i:i + 3] != b"EOF":
        offset = int.from_bytes(patch[i:i + 3], "big")
        size = int.from_bytes(patch[i + 3:i + 5], "big")
        data = patch[i + 5:i + 5 + size]
        end = offset + size
        if end > len(out):
            out.extend(b"\x00" * (end - len(out)))
        out[offset:end] = data
        i += 5 + size

    return bytes(out)


def test_create_patch_roundtrip(tmp_path: Path):
    base = bytes([0, 1, 2, 3, 4, 5, 6, 7])
    target = bytes([0, 1, 9, 3, 4, 5, 6, 8, 9, 10])

    base_path = tmp_path / "base.gba"
    target_path = tmp_path / "target.gba"
    patch_path = tmp_path / "out.ips"

    base_path.write_bytes(base)
    target_path.write_bytes(target)

    make_ips_patch.create_patch(base_path, target_path, patch_path)

    patch = patch_path.read_bytes()
    rebuilt = _apply_ips(base, patch)

    assert rebuilt == target
