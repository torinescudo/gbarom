#!/usr/bin/env python3
"""Smoke test for relay.py.

Runs an in-process server, connects two clients, and verifies frame relay.
"""

from __future__ import annotations

import asyncio
import json

from protocol import PROTOCOL_VERSION, encode_packet
from relay import run_server


async def read_json_line(reader: asyncio.StreamReader) -> dict:
    line = await asyncio.wait_for(reader.readline(), timeout=2)
    if not line:
        raise RuntimeError("connection_closed")
    return json.loads(line.decode("utf-8"))


async def write_packet(writer: asyncio.StreamWriter, payload: dict) -> None:
    writer.write(encode_packet(payload))
    await writer.drain()


async def client_join(name: str, side: int):
    reader, writer = await asyncio.open_connection("127.0.0.1", 9876)

    await read_json_line(reader)  # welcome
    await write_packet(writer, {"type": "hello", "version": PROTOCOL_VERSION, "name": name})
    ack = await read_json_line(reader)
    assert ack["type"] == "hello_ack", ack

    await write_packet(writer, {"type": "join", "room": "smoke", "side": side})
    joined = await read_json_line(reader)
    assert joined["type"] == "joined", joined

    return reader, writer


async def main() -> None:
    server_task = asyncio.create_task(run_server("127.0.0.1", 9876, 60))
    await asyncio.sleep(0.15)

    a_reader, a_writer = await client_join("alice", 0)
    b_reader, b_writer = await client_join("bob", 1)

    # Drain peer_update/start messages.
    for _ in range(2):
        await read_json_line(a_reader)
    for _ in range(2):
        await read_json_line(b_reader)

    await write_packet(
        a_writer,
        {
            "type": "link_frame",
            "room": "smoke",
            "seq": 1,
            "payload": "abcd",
        },
    )

    relayed = await read_json_line(b_reader)
    assert relayed["type"] == "link_frame", relayed
    assert relayed["from"] == 0, relayed
    assert relayed["seq"] == 1, relayed
    assert relayed["payload"] == "abcd", relayed

    a_writer.close()
    b_writer.close()
    await a_writer.wait_closed()
    await b_writer.wait_closed()

    server_task.cancel()
    try:
        await server_task
    except asyncio.CancelledError:
        pass


if __name__ == "__main__":
    asyncio.run(main())
