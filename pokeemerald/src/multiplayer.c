#include "global.h"
#include "multiplayer.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "overworld.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/maps.h"

/*
 * multiplayer.c — Full overworld multiplayer for pokeemerald
 *
 * This module manages:
 *   1. Connection lifecycle via abstract transport
 *   2. Local player state broadcast (position, sprite, direction)
 *   3. Remote player avatar spawn/update/despawn on any map
 *   4. On-the-spot interaction (battle/trade/talk) from any tile
 *   5. Transition into link battle/trade using vanilla pokeemerald subsystems
 */

// ── State ─────────────────────────────────────────────────

static EWRAM_DATA enum MultiplayerState sState = MP_STATE_OFFLINE;
static EWRAM_DATA struct RemotePlayerState sRemoteState = {0};
static EWRAM_DATA struct RemotePlayerState sLocalStatePrev = {0};
static EWRAM_DATA u8 sRemoteObjEventId = OBJECT_EVENTS_COUNT;
static EWRAM_DATA bool8 sRemoteAvatarSpawned = FALSE;
static EWRAM_DATA u8 sSendSeq = 0;
static EWRAM_DATA u8 sRecvSeq = 0;
static EWRAM_DATA u16 sTickCounter = 0;
static EWRAM_DATA u16 sPingTimer = 0;
static EWRAM_DATA enum MultiplayerInteraction sPendingInteraction = MP_INTERACT_NONE;
static EWRAM_DATA bool8 sInteractionIncoming = FALSE;
static EWRAM_DATA u8 sRoomCode[16] = {0};

// ── Forward declarations ──────────────────────────────────

static void ProcessIncomingPackets(void);
static void BroadcastLocalState(void);
static void HandleRemoteStateUpdate(const struct RemotePlayerState *remote);
static void SendPacket(u8 type, const void *data, u16 size);
static struct RemotePlayerState BuildLocalState(void);
static bool8 LocalStateChanged(const struct RemotePlayerState *a, const struct RemotePlayerState *b);
static u8 GetPlayerGraphicsId(void);

// ── Lifecycle ─────────────────────────────────────────────

void Multiplayer_Init(void)
{
    sState = MP_STATE_OFFLINE;
    sRemoteObjEventId = OBJECT_EVENTS_COUNT;
    sRemoteAvatarSpawned = FALSE;
    sSendSeq = 0;
    sRecvSeq = 0;
    sTickCounter = 0;
    sPingTimer = 0;
    sPendingInteraction = MP_INTERACT_NONE;
    sInteractionIncoming = FALSE;
}

void Multiplayer_Shutdown(void)
{
    Multiplayer_DespawnRemoteAvatar();
    sState = MP_STATE_OFFLINE;
}

void Multiplayer_Tick(void)
{
    if (sState == MP_STATE_OFFLINE || sState == MP_STATE_ERROR)
        return;

    sTickCounter++;

    // Process incoming packets from transport
    ProcessIncomingPackets();

    // Broadcast our position every frame (delta-compressed)
    if (sState >= MP_STATE_SYNCED)
        BroadcastLocalState();

    // Ping every 60 frames (~1 second)
    sPingTimer++;
    if (sPingTimer >= 60)
    {
        struct MultiplayerPacket pkt = {0};
        pkt.type = MP_MSG_PING;
        pkt.seq = sSendSeq++;
        // Embed tick counter for RTT measurement
        pkt.payload[0] = (u8)(sTickCounter & 0xFF);
        pkt.payload[1] = (u8)(sTickCounter >> 8);
        pkt.size = 2;
        MpTransport_Send(&pkt);
        sPingTimer = 0;
    }

    // Update remote avatar position smoothly
    if (sRemoteAvatarSpawned)
        Multiplayer_UpdateRemoteAvatar();
}

// ── Connection ────────────────────────────────────────────

void Multiplayer_Connect(const u8 *roomCode)
{
    u32 i;

    if (roomCode == NULL)
        return;

    for (i = 0; i < sizeof(sRoomCode) - 1 && roomCode[i] != 0xFF && roomCode[i] != 0; i++)
        sRoomCode[i] = roomCode[i];
    sRoomCode[i] = 0;

    sState = MP_STATE_CONNECTING;

    // Send hello
    SendPacket(MP_MSG_HELLO, NULL, 0);

    // Send join room
    SendPacket(MP_MSG_JOIN_ROOM, sRoomCode, i);

    sState = MP_STATE_IN_ROOM;
}

void Multiplayer_Disconnect(void)
{
    SendPacket(MP_MSG_LEAVE_ROOM, NULL, 0);
    Multiplayer_DespawnRemoteAvatar();
    sState = MP_STATE_OFFLINE;
}

enum MultiplayerState Multiplayer_GetState(void)
{
    return sState;
}

bool8 Multiplayer_IsConnected(void)
{
    return sState >= MP_STATE_IN_ROOM;
}

bool8 Multiplayer_IsPeerOnSameMap(void)
{
    if (sState < MP_STATE_SYNCED)
        return FALSE;

    return (sRemoteState.mapGroup == gSaveBlock1Ptr->location.mapGroup
         && sRemoteState.mapNum == gSaveBlock1Ptr->location.mapNum);
}

// ── Remote Avatar ─────────────────────────────────────────

void Multiplayer_SpawnRemoteAvatar(void)
{
    u8 graphicsId;
    u8 objId;

    if (sRemoteAvatarSpawned)
        return;

    graphicsId = sRemoteState.graphicsId;
    if (graphicsId == 0)
        graphicsId = (sRemoteState.gender == MALE) ? OBJ_EVENT_GFX_BRENDAN_NORMAL : OBJ_EVENT_GFX_MAY_NORMAL;

    objId = SpawnSpecialObjectEventParameterized(
        graphicsId,
        MOVEMENT_TYPE_FACE_DOWN,  // Will be overridden immediately
        LOCALID_REMOTE_PLAYER,
        sRemoteState.x + MAP_OFFSET,
        sRemoteState.y + MAP_OFFSET,
        sRemoteState.elevation
    );

    if (objId < OBJECT_EVENTS_COUNT)
    {
        sRemoteObjEventId = objId;
        sRemoteAvatarSpawned = TRUE;
        gObjectEvents[objId].invisible = FALSE;
        gObjectEvents[objId].isPlayer = FALSE;  // Not local player
    }
}

void Multiplayer_DespawnRemoteAvatar(void)
{
    if (!sRemoteAvatarSpawned)
        return;

    if (sRemoteObjEventId < OBJECT_EVENTS_COUNT)
    {
        RemoveObjectEventByLocalIdAndMap(
            LOCALID_REMOTE_PLAYER,
            gSaveBlock1Ptr->location.mapNum,
            gSaveBlock1Ptr->location.mapGroup
        );
    }

    sRemoteObjEventId = OBJECT_EVENTS_COUNT;
    sRemoteAvatarSpawned = FALSE;
}

void Multiplayer_UpdateRemoteAvatar(void)
{
    struct ObjectEvent *obj;

    if (!sRemoteAvatarSpawned || sRemoteObjEventId >= OBJECT_EVENTS_COUNT)
        return;

    obj = &gObjectEvents[sRemoteObjEventId];

    // If remote player changed map, despawn/respawn
    if (!Multiplayer_IsPeerOnSameMap())
    {
        Multiplayer_DespawnRemoteAvatar();
        return;
    }

    // Update position
    if (obj->currentCoords.x != sRemoteState.x || obj->currentCoords.y != sRemoteState.y)
    {
        MoveObjectEventToMapCoords(obj, sRemoteState.x, sRemoteState.y);
    }

    // Update facing direction
    if (obj->facingDirection != sRemoteState.facingDirection)
    {
        SetObjectEventDirection(obj, sRemoteState.facingDirection);
    }

    // Update graphics if changed (switching to bike, surfing, etc.)
    if (obj->graphicsId != sRemoteState.graphicsId && sRemoteState.graphicsId != 0)
    {
        ObjectEventSetGraphicsId(obj, sRemoteState.graphicsId);
    }

    obj->invisible = FALSE;
}

u8 Multiplayer_GetRemoteObjectEventId(void)
{
    return sRemoteObjEventId;
}

// ── Local state broadcast ─────────────────────────────────

static struct RemotePlayerState BuildLocalState(void)
{
    struct RemotePlayerState state = {0};
    struct ObjectEvent *playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];

    state.mapGroup = gSaveBlock1Ptr->location.mapGroup;
    state.mapNum = gSaveBlock1Ptr->location.mapNum;
    state.x = playerObj->currentCoords.x;
    state.y = playerObj->currentCoords.y;
    state.facingDirection = playerObj->facingDirection;
    state.movementAction = playerObj->movementActionId;
    state.graphicsId = GetPlayerGraphicsId();
    state.gender = gSaveBlock2Ptr->playerGender;
    state.elevation = playerObj->currentElevation;
    state.flags = gPlayerAvatar.flags;
    state.trainerIdLow = (u16)(gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8));
    state.interactionRequest = (u8)sPendingInteraction;

    // Copy player name
    {
        u32 i;
        for (i = 0; i < PLAYER_NAME_LENGTH; i++)
            state.name[i] = gSaveBlock2Ptr->playerName[i];
        state.name[PLAYER_NAME_LENGTH] = 0xFF;
    }

    return state;
}

static bool8 LocalStateChanged(const struct RemotePlayerState *a, const struct RemotePlayerState *b)
{
    if (a->mapGroup != b->mapGroup || a->mapNum != b->mapNum)
        return TRUE;
    if (a->x != b->x || a->y != b->y)
        return TRUE;
    if (a->facingDirection != b->facingDirection)
        return TRUE;
    if (a->graphicsId != b->graphicsId)
        return TRUE;
    if (a->elevation != b->elevation)
        return TRUE;
    if (a->flags != b->flags)
        return TRUE;
    if (a->interactionRequest != b->interactionRequest)
        return TRUE;
    return FALSE;
}

static void BroadcastLocalState(void)
{
    struct RemotePlayerState current = BuildLocalState();

    // Only send when something changed (delta compression)
    if (LocalStateChanged(&current, &sLocalStatePrev))
    {
        SendPacket(MP_MSG_PLAYER_STATE, &current, sizeof(current));
        sLocalStatePrev = current;
    }
}

void Multiplayer_SendLocalState(void)
{
    struct RemotePlayerState current = BuildLocalState();
    SendPacket(MP_MSG_PLAYER_STATE, &current, sizeof(current));
    sLocalStatePrev = current;
}

// ── Interaction system ────────────────────────────────────

void Multiplayer_RequestInteraction(enum MultiplayerInteraction type)
{
    u8 data[1];

    if (sState < MP_STATE_SYNCED)
        return;

    data[0] = (u8)type;
    SendPacket(MP_MSG_INTERACT_REQ, data, 1);
    sPendingInteraction = type;
}

bool8 Multiplayer_HasPendingInteraction(void)
{
    return sInteractionIncoming;
}

enum MultiplayerInteraction Multiplayer_GetPendingInteraction(void)
{
    return sPendingInteraction;
}

void Multiplayer_AcceptInteraction(void)
{
    u8 data[1];
    data[0] = (u8)sPendingInteraction;
    SendPacket(MP_MSG_INTERACT_ACK, data, 1);

    switch (sPendingInteraction)
    {
    case MP_INTERACT_BATTLE:
        Multiplayer_InitBattle();
        break;
    case MP_INTERACT_TRADE:
        Multiplayer_InitTrade();
        break;
    default:
        break;
    }

    sInteractionIncoming = FALSE;
}

void Multiplayer_DeclineInteraction(void)
{
    sPendingInteraction = MP_INTERACT_NONE;
    sInteractionIncoming = FALSE;
}

// ── Battle/Trade on the spot ──────────────────────────────

void Multiplayer_InitBattle(void)
{
    /*
     * Transition to link battle using pokeemerald's existing system.
     * We set up the link state as if a cable were connected:
     *
     * 1. Set gLinkType to the appropriate battle type.
     * 2. Populate gLinkPlayers[1] with the remote player's data.
     * 3. Set link flags so BattleSetup thinks we have a valid link.
     * 4. Call the battle setup function.
     *
     * The actual battle data exchange goes through our relay
     * (MpTransport_Send/Recv wrapping MP_MSG_BATTLE_DATA).
     *
     * TODO: Implement the full battle data bridge.
     * For now this is a framework that shows where each piece connects.
     */
    sState = MP_STATE_BATTLE;
}

void Multiplayer_InitTrade(void)
{
    /*
     * Same approach as battle: wire up gLinkPlayers and gLinkType
     * for trade, then let pokeemerald's trade code run while we
     * bridge the data through the relay.
     *
     * TODO: Implement the full trade data bridge.
     */
    sState = MP_STATE_TRADE;
}

// ── Packet processing ─────────────────────────────────────

static void HandleRemoteStateUpdate(const struct RemotePlayerState *remote)
{
    bool8 wasOnSameMap;
    bool8 nowOnSameMap;

    wasOnSameMap = Multiplayer_IsPeerOnSameMap();

    sRemoteState = *remote;

    // If just entered synced state
    if (sState == MP_STATE_IN_ROOM)
        sState = MP_STATE_SYNCED;

    nowOnSameMap = Multiplayer_IsPeerOnSameMap();

    // Spawn avatar if peer just arrived on our map
    if (!wasOnSameMap && nowOnSameMap)
        Multiplayer_SpawnRemoteAvatar();

    // Despawn if peer left our map
    if (wasOnSameMap && !nowOnSameMap)
        Multiplayer_DespawnRemoteAvatar();

    // Check for interaction request
    if (remote->interactionRequest != MP_INTERACT_NONE && remote->interactionRequest != (u8)sPendingInteraction)
    {
        sPendingInteraction = (enum MultiplayerInteraction)remote->interactionRequest;
        sInteractionIncoming = TRUE;
    }
}

static void ProcessIncomingPackets(void)
{
    struct MultiplayerPacket pkt;
    u32 maxPerTick = 8;  // Process up to 8 packets per frame

    while (maxPerTick-- > 0 && MpTransport_Recv(&pkt))
    {
        switch (pkt.type)
        {
        case MP_MSG_PLAYER_STATE:
            if (pkt.size >= sizeof(struct RemotePlayerState))
                HandleRemoteStateUpdate((const struct RemotePlayerState *)pkt.payload);
            break;

        case MP_MSG_INTERACT_REQ:
            if (pkt.size >= 1)
            {
                sPendingInteraction = (enum MultiplayerInteraction)pkt.payload[0];
                sInteractionIncoming = TRUE;
            }
            break;

        case MP_MSG_INTERACT_ACK:
            if (pkt.size >= 1)
            {
                enum MultiplayerInteraction ack = (enum MultiplayerInteraction)pkt.payload[0];
                if (ack == MP_INTERACT_BATTLE)
                    Multiplayer_InitBattle();
                else if (ack == MP_INTERACT_TRADE)
                    Multiplayer_InitTrade();
            }
            break;

        case MP_MSG_PONG:
            // RTT measurement — could store for display
            break;

        case MP_MSG_BATTLE_DATA:
            // Forward to battle subsystem
            // TODO: Inject into link recv queue
            break;

        case MP_MSG_TRADE_DATA:
            // Forward to trade subsystem
            // TODO: Inject into link recv queue
            break;

        default:
            break;
        }
    }
}

// ── Transport helpers ─────────────────────────────────────

static void SendPacket(u8 type, const void *data, u16 size)
{
    struct MultiplayerPacket pkt = {0};

    pkt.type = type;
    pkt.seq = sSendSeq++;
    pkt.size = (size <= MP_MAX_PAYLOAD) ? size : MP_MAX_PAYLOAD;

    if (data != NULL && pkt.size > 0)
    {
        u32 i;
        const u8 *src = (const u8 *)data;
        for (i = 0; i < pkt.size; i++)
            pkt.payload[i] = src[i];
    }

    MpTransport_Send(&pkt);
}

static u8 GetPlayerGraphicsId(void)
{
    u8 flags = gPlayerAvatar.flags;

    if (gSaveBlock2Ptr->playerGender == MALE)
    {
        if (flags & PLAYER_AVATAR_FLAG_MACH_BIKE)
            return OBJ_EVENT_GFX_BRENDAN_MACH_BIKE;
        if (flags & PLAYER_AVATAR_FLAG_ACRO_BIKE)
            return OBJ_EVENT_GFX_BRENDAN_ACRO_BIKE;
        if (flags & PLAYER_AVATAR_FLAG_SURFING)
            return OBJ_EVENT_GFX_BRENDAN_SURFING;
        return OBJ_EVENT_GFX_BRENDAN_NORMAL;
    }
    else
    {
        if (flags & PLAYER_AVATAR_FLAG_MACH_BIKE)
            return OBJ_EVENT_GFX_MAY_MACH_BIKE;
        if (flags & PLAYER_AVATAR_FLAG_ACRO_BIKE)
            return OBJ_EVENT_GFX_MAY_ACRO_BIKE;
        if (flags & PLAYER_AVATAR_FLAG_SURFING)
            return OBJ_EVENT_GFX_MAY_SURFING;
        return OBJ_EVENT_GFX_MAY_NORMAL;
    }
}

// ── Transport stub (loopback for testing) ─────────────────
// In production, the bridge (emulator/hardware) implements these.

#define MP_LOOPBACK_QUEUE_SIZE 32

static EWRAM_DATA struct MultiplayerPacket sLoopbackQueue[MP_LOOPBACK_QUEUE_SIZE] = {0};
static EWRAM_DATA u8 sLoopbackHead = 0;
static EWRAM_DATA u8 sLoopbackTail = 0;
static EWRAM_DATA u8 sLoopbackCount = 0;

bool8 MpTransport_Send(const struct MultiplayerPacket *pkt)
{
    if (pkt == NULL || sLoopbackCount >= MP_LOOPBACK_QUEUE_SIZE)
        return FALSE;

    sLoopbackQueue[sLoopbackTail] = *pkt;
    sLoopbackTail = (sLoopbackTail + 1) % MP_LOOPBACK_QUEUE_SIZE;
    sLoopbackCount++;
    return TRUE;
}

bool8 MpTransport_Recv(struct MultiplayerPacket *pktOut)
{
    if (pktOut == NULL || sLoopbackCount == 0)
        return FALSE;

    *pktOut = sLoopbackQueue[sLoopbackHead];
    sLoopbackHead = (sLoopbackHead + 1) % MP_LOOPBACK_QUEUE_SIZE;
    sLoopbackCount--;
    return TRUE;
}

bool8 MpTransport_IsReady(void)
{
    // Stub: always ready for loopback
    return TRUE;
}
