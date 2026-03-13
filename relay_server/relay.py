#!/usr/bin/env python3
"""Minimal online relay for pokeemerald link traffic.

This server is intentionally small and dependency-free so it can be deployed
quickly while protocol and emulator bridge are still evolving.
"""

from __future__ import annotations

import argparse
import asyncio
import secrets
from dataclasses import dataclass
from typing import Dict, Optional

from protocol import PROTOCOL_VERSION, Packet, ProtocolError, decode_packet, encode_packet


@dataclass
class Peer:
    reader: asyncio.StreamReader
    writer: asyncio.StreamWriter
    peer_name: str = "unknown"
    room_id: Optional[str] = None
    side: Optional[int] = None
    version_ok: bool = False


@dataclass
class Room:
    room_id: str
    seed: int
    tick_hz: int
    peers: Dict[int, Peer]


class RelayServer:
    def __init__(self, tick_hz: int = 60) -> None:
        self.tick_hz = tick_hz
        self.rooms: Dict[str, Room] = {}

    async def handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        peer = Peer(reader=reader, writer=writer)
        addr = writer.get_extra_info("peername")
        print(f"[connect] {addr}")

        await self._send(peer, {"type": "welcome", "protocol": PROTOCOL_VERSION})

        try:
            while True:
                line = await reader.readline()
                if not line:
                    break

                try:
                    packet = decode_packet(line)
                except ProtocolError as exc:
                    await self._send(peer, {"type": "error", "code": str(exc)})
                    continue

                await self._dispatch(peer, packet)
        except asyncio.CancelledError:
            raise
        except Exception as exc:  # pragma: no cover
            print(f"[error] client_loop: {exc}")
        finally:
            self._remove_peer(peer)
            writer.close()
            await writer.wait_closed()
            print(f"[disconnect] {addr}")

    async def _dispatch(self, peer: Peer, packet: Packet) -> None:
        data = packet.payload

        if packet.type == "hello":
            await self._on_hello(peer, data)
            return

        if not peer.version_ok:
            await self._send(peer, {"type": "error", "code": "hello_required"})
            return

        if packet.type == "join":
            await self._on_join(peer, data)
            return

        if packet.type == "leave":
            self._remove_peer(peer)
            await self._send(peer, {"type": "left"})
            return

        if packet.type == "ping":
            await self._send(peer, {"type": "pong", "ts": data.get("ts")})
            return

        if packet.type in {"link_frame", "input", "state_hash",
                          "player_state", "interact_request",
                          "interact_accept", "interact_decline",
                          "battle_init", "trade_init"}:
            await self._relay_to_room(peer, data)
            return

        await self._send(peer, {"type": "error", "code": "unknown_type"})

    async def _on_hello(self, peer: Peer, data: dict) -> None:
        version = str(data.get("version", ""))
        if version != PROTOCOL_VERSION:
            await self._send(
                peer,
                {
                    "type": "error",
                    "code": "version_mismatch",
                    "expected": PROTOCOL_VERSION,
                    "got": version,
                },
            )
            return

        peer.peer_name = str(data.get("name", "unknown"))
        peer.version_ok = True
        await self._send(peer, {"type": "hello_ack", "name": peer.peer_name})

    async def _on_join(self, peer: Peer, data: dict) -> None:
        room_id = str(data.get("room", "")).strip()
        if not room_id:
            await self._send(peer, {"type": "error", "code": "invalid_room"})
            return

        requested_side = data.get("side")
        if requested_side is not None and requested_side not in (0, 1):
            await self._send(peer, {"type": "error", "code": "invalid_side"})
            return

        self._remove_peer(peer)

        room = self.rooms.get(room_id)
        if room is None:
            room = Room(
                room_id=room_id,
                seed=secrets.randbits(32),
                tick_hz=self.tick_hz,
                peers={},
            )
            self.rooms[room_id] = room

        if len(room.peers) >= 2 and requested_side not in room.peers:
            await self._send(peer, {"type": "error", "code": "room_full"})
            return

        if requested_side is None:
            side = 0 if 0 not in room.peers else 1
        else:
            side = requested_side

        if side in room.peers:
            await self._send(peer, {"type": "error", "code": "side_taken"})
            return

        room.peers[side] = peer
        peer.room_id = room_id
        peer.side = side

        await self._send(peer, {"type": "joined", "room": room_id, "side": side})
        await self._broadcast(
            room,
            {
                "type": "peer_update",
                "room": room_id,
                "players": sorted(room.peers.keys()),
            },
        )

        if len(room.peers) == 2:
            await self._broadcast(
                room,
                {
                    "type": "start",
                    "room": room_id,
                    "seed": room.seed,
                    "tick_hz": room.tick_hz,
                },
            )

    async def _relay_to_room(self, source: Peer, data: dict) -> None:
        room = self._get_peer_room(source)
        if room is None:
            await self._send(source, {"type": "error", "code": "not_in_room"})
            return

        relay_data = dict(data)
        relay_data["from"] = source.side
        await self._broadcast(room, relay_data, exclude=source)

    def _remove_peer(self, peer: Peer) -> None:
        if peer.room_id is None or peer.side is None:
            return

        room = self.rooms.get(peer.room_id)
        if room is None:
            peer.room_id = None
            peer.side = None
            return

        room.peers.pop(peer.side, None)
        room_id = room.room_id

        if room.peers:
            asyncio.create_task(
                self._broadcast(
                    room,
                    {
                        "type": "peer_update",
                        "room": room_id,
                        "players": sorted(room.peers.keys()),
                    },
                )
            )
        else:
            self.rooms.pop(room_id, None)

        peer.room_id = None
        peer.side = None

    def _get_peer_room(self, peer: Peer) -> Optional[Room]:
        if peer.room_id is None:
            return None
        return self.rooms.get(peer.room_id)

    async def _broadcast(self, room: Room, packet: dict, exclude: Optional[Peer] = None) -> None:
        payload = encode_packet(packet)
        dead_peers = []

        for p in room.peers.values():
            if exclude is not None and p is exclude:
                continue
            try:
                p.writer.write(payload)
                await p.writer.drain()
            except Exception:
                dead_peers.append(p)

        for dead_peer in dead_peers:
            self._remove_peer(dead_peer)

    async def _send(self, peer: Peer, packet: dict) -> None:
        peer.writer.write(encode_packet(packet))
        await peer.writer.drain()


async def run_server(host: str, port: int, tick_hz: int) -> None:
    relay = RelayServer(tick_hz=tick_hz)
    server = await asyncio.start_server(relay.handle_client, host, port)
    addrs = ", ".join(str(sock.getsockname()) for sock in server.sockets or [])
    print(f"[listen] {addrs}")

    async with server:
        await server.serve_forever()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="pokeemerald online relay")
    parser.add_argument("--host", default="0.0.0.0", help="Bind host")
    parser.add_argument("--port", type=int, default=8765, help="Bind port")
    parser.add_argument("--tick-hz", type=int, default=60, help="Target session tick")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    asyncio.run(run_server(host=args.host, port=args.port, tick_hz=args.tick_hz))


if __name__ == "__main__":
    main()
