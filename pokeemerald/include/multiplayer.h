#ifndef GUARD_MULTIPLAYER_H
#define GUARD_MULTIPLAYER_H

#include "global.h"

/*
 * Full-overworld multiplayer system for pokeemerald.
 *
 * Allows two players to see each other on ANY map, interact,
 * battle and trade on the spot — via relay server (emulator web
 * or hardware WiFi bridge).
 *
 * Architecture:
 *   ROM ←→ MultiplayerTransport (abstract) ←→ Bridge ←→ Relay Server ←→ Bridge ←→ ROM
 *
 * The transport layer is pluggable:
 *   - Emulator web: WebSocket bridge in JS intercepts SIO writes
 *   - Hardware: ESP32/RP2040 on GBA link port wraps to WiFi
 */

// ── Connection states ─────────────────────────────────────
enum MultiplayerState
{
    MP_STATE_OFFLINE = 0,
    MP_STATE_CONNECTING,
    MP_STATE_LOBBY,       // Connected but not in a room
    MP_STATE_IN_ROOM,     // In room, waiting for peer
    MP_STATE_SYNCED,      // Both players active, syncing overworld
    MP_STATE_BATTLE,      // In a link battle
    MP_STATE_TRADE,       // In a link trade
    MP_STATE_ERROR,
};

// ── Interaction types ─────────────────────────────────────
enum MultiplayerInteraction
{
    MP_INTERACT_NONE = 0,
    MP_INTERACT_TALK,
    MP_INTERACT_BATTLE,
    MP_INTERACT_TRADE,
    MP_INTERACT_SHOW_CARD,
};

// ── Remote player overworld data ──────────────────────────
// Sent every frame when position/state changes.
struct RemotePlayerState
{
    u16 mapGroup;
    u16 mapNum;
    s16 x;
    s16 y;
    u8  facingDirection;
    u8  movementAction;    // Current movement animation
    u8  graphicsId;        // Sprite (Brendan/May/bike/surf)
    u8  gender;
    u8  elevation;
    u8  flags;             // Running, surfing, biking, etc.
    u16 trainerIdLow;      // For display
    u8  name[PLAYER_NAME_LENGTH + 1];
    u8  interactionRequest; // MP_INTERACT_*
    u8  padding;
};

// ── Protocol message types ────────────────────────────────
#define MP_MSG_HELLO        0x01
#define MP_MSG_JOIN_ROOM    0x02
#define MP_MSG_LEAVE_ROOM   0x03
#define MP_MSG_PLAYER_STATE 0x04
#define MP_MSG_INTERACT_REQ 0x05
#define MP_MSG_INTERACT_ACK 0x06
#define MP_MSG_BATTLE_DATA  0x07
#define MP_MSG_TRADE_DATA   0x08
#define MP_MSG_PING         0x09
#define MP_MSG_PONG         0x0A
#define MP_MSG_CHAT         0x0B
#define MP_MSG_STATE_HASH   0x0C

// ── Transport packet ──────────────────────────────────────
#define MP_MAX_PAYLOAD 64

struct MultiplayerPacket
{
    u8  type;
    u8  seq;
    u16 size;
    u8  payload[MP_MAX_PAYLOAD];
};

// ── Remote player local ID for ObjectEvent ────────────────
#define LOCALID_REMOTE_PLAYER 254
#define REMOTE_PLAYER_OBJ_IDX 15  // Use last ObjectEvent slot

// ── Public API ────────────────────────────────────────────

// Lifecycle
void Multiplayer_Init(void);
void Multiplayer_Shutdown(void);
void Multiplayer_Tick(void);  // Call every VBlank in overworld

// Connection
void Multiplayer_Connect(const u8 *roomCode);
void Multiplayer_Disconnect(void);
enum MultiplayerState Multiplayer_GetState(void);
bool8 Multiplayer_IsConnected(void);
bool8 Multiplayer_IsPeerOnSameMap(void);

// Remote player avatar
void Multiplayer_SpawnRemoteAvatar(void);
void Multiplayer_DespawnRemoteAvatar(void);
void Multiplayer_UpdateRemoteAvatar(void);
u8   Multiplayer_GetRemoteObjectEventId(void);

// Local state broadcast
void Multiplayer_SendLocalState(void);

// Interaction
void Multiplayer_RequestInteraction(enum MultiplayerInteraction type);
bool8 Multiplayer_HasPendingInteraction(void);
enum MultiplayerInteraction Multiplayer_GetPendingInteraction(void);
void Multiplayer_AcceptInteraction(void);
void Multiplayer_DeclineInteraction(void);

// Battle/Trade on the spot
void Multiplayer_InitBattle(void);
void Multiplayer_InitTrade(void);

// Transport (abstract — implemented by bridge)
bool8 MpTransport_Send(const struct MultiplayerPacket *pkt);
bool8 MpTransport_Recv(struct MultiplayerPacket *pktOut);
bool8 MpTransport_IsReady(void);

#endif // GUARD_MULTIPLAYER_H
