#!/usr/bin/env python3
"""WebSocket bridge for the GBA online relay.

Bridges browser-based emulators (WebSocket) ↔ TCP relay server.
Each WS client gets proxied to the relay as a TCP peer.

Usage:
    python ws_bridge.py --ws-port 8765 --relay-host 127.0.0.1 --relay-port 9001
"""

from __future__ import annotations

import argparse
import asyncio
import json
import signal

try:
    import websockets
    from websockets.asyncio.server import serve
except ImportError:
    print("Install websockets: pip install websockets")
    raise SystemExit(1)


async def bridge_client(ws, relay_host: str, relay_port: int) -> None:
    """Handle one WS client, proxy to/from the TCP relay."""
    reader, writer = await asyncio.open_connection(relay_host, relay_port)
    print(f"[ws_bridge] new client, connected to relay {relay_host}:{relay_port}")

    async def relay_to_ws() -> None:
        """Read JSONL lines from TCP relay, forward to WS."""
        try:
            while True:
                line = await reader.readline()
                if not line:
                    break
                await ws.send(line.decode("utf-8").rstrip("\n"))
        except asyncio.CancelledError:
            pass

    async def ws_to_relay() -> None:
        """Read WS messages, forward as JSONL to TCP relay."""
        try:
            async for msg in ws:
                if isinstance(msg, bytes):
                    msg = msg.decode("utf-8")
                writer.write((msg.rstrip("\n") + "\n").encode("utf-8"))
                await writer.drain()
        except asyncio.CancelledError:
            pass

    task_r2w = asyncio.create_task(relay_to_ws())
    task_w2r = asyncio.create_task(ws_to_relay())

    try:
        done, pending = await asyncio.wait(
            [task_r2w, task_w2r], return_when=asyncio.FIRST_COMPLETED
        )
    finally:
        task_r2w.cancel()
        task_w2r.cancel()
        writer.close()
        await writer.wait_closed()
        print("[ws_bridge] client disconnected")


async def run_bridge(ws_host: str, ws_port: int, relay_host: str, relay_port: int) -> None:
    async def handler(ws):
        await bridge_client(ws, relay_host, relay_port)

    async with serve(handler, ws_host, ws_port) as server:
        print(f"[ws_bridge] listening on ws://{ws_host}:{ws_port}")
        stop = asyncio.get_event_loop().create_future()

        def on_signal():
            if not stop.done():
                stop.set_result(True)

        loop = asyncio.get_event_loop()
        for sig in (signal.SIGINT, signal.SIGTERM):
            loop.add_signal_handler(sig, on_signal)

        await stop


def main() -> None:
    parser = argparse.ArgumentParser(description="WS ↔ TCP relay bridge")
    parser.add_argument("--ws-host", default="0.0.0.0")
    parser.add_argument("--ws-port", type=int, default=8765)
    parser.add_argument("--relay-host", default="127.0.0.1")
    parser.add_argument("--relay-port", type=int, default=9001)
    args = parser.parse_args()

    asyncio.run(run_bridge(args.ws_host, args.ws_port, args.relay_host, args.relay_port))


if __name__ == "__main__":
    main()
