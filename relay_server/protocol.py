"""Protocol helpers for the GBA online relay.

Phase-1 protocol uses JSON lines over TCP to simplify debugging.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from typing import Any, Dict

PROTOCOL_VERSION = "1"
MAX_LINE_BYTES = 8 * 1024


class ProtocolError(ValueError):
    """Raised when an incoming packet is malformed."""


@dataclass(frozen=True)
class Packet:
    type: str
    payload: Dict[str, Any]


def encode_packet(packet: Dict[str, Any]) -> bytes:
    raw = json.dumps(packet, separators=(",", ":"), ensure_ascii=True)
    return (raw + "\n").encode("utf-8")


def decode_packet(line: bytes) -> Packet:
    if len(line) > MAX_LINE_BYTES:
        raise ProtocolError("line_too_long")

    try:
        obj = json.loads(line.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ProtocolError("invalid_json") from exc

    if not isinstance(obj, dict):
        raise ProtocolError("packet_not_object")

    packet_type = obj.get("type")
    if not isinstance(packet_type, str) or not packet_type:
        raise ProtocolError("missing_type")

    return Packet(type=packet_type, payload=obj)
