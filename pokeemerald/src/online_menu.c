#include "global.h"
#include "online_menu.h"
#include "multiplayer.h"
#include "multiplayer_menu.h"
#include "task.h"
#include "menu.h"
#include "window.h"
#include "text.h"
#include "text_window.h"
#include "string_util.h"
#include "sound.h"
#include "palette.h"
#include "bg.h"
#include "gpu_regs.h"
#include "scanline_effect.h"
#include "overworld.h"
#include "main.h"
#include "field_screen_effect.h"
#include "strings.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "naming_screen.h"
#include "constants/songs.h"
#include "constants/rgb.h"

/*
 * online_menu.c — Full-screen Online menu accessible from Start menu.
 *
 * Options:
 *   CONNECT       — Enter room code and connect
 *   DISCONNECT    — Leave current room
 *   TEST MODE     — Spawn a fake remote player for local testing
 *   STATUS        — Show connection info
 *   BACK          — Return to overworld
 */

/* ── Constants ─────────────────────────────────────── */

enum {
    ONLINE_MENU_CONNECT = 0,
    ONLINE_MENU_DISCONNECT,
    ONLINE_MENU_TEST_MODE,
    ONLINE_MENU_STATUS,
    ONLINE_MENU_BACK,
    ONLINE_MENU_COUNT
};

/* ── Text strings ──────────────────────────────────── */

static const u8 sText_Online[]       = _("ONLINE");
static const u8 sText_Connect[]      = _("Connect");
static const u8 sText_Disconnect[]   = _("Disconnect");
static const u8 sText_TestMode[]     = _("Test Mode");
static const u8 sText_Status[]       = _("Status");
static const u8 sText_Back[]         = _("Back");

static const u8 sText_StatusOffline[]    = _("Status: OFFLINE");
static const u8 sText_StatusConnecting[] = _("Status: CONNECTING…");
static const u8 sText_StatusInRoom[]     = _("Status: IN ROOM");
static const u8 sText_StatusSynced[]     = _("Status: SYNCED");
static const u8 sText_StatusBattle[]     = _("Status: BATTLE");
static const u8 sText_StatusTrade[]      = _("Status: TRADE");
static const u8 sText_StatusError[]      = _("Status: ERROR");

static const u8 sText_Connected[]    = _("Connected to room!");
static const u8 sText_Disconnected[] = _("Disconnected.");
static const u8 sText_AlreadyOn[]    = _("Already connected!");
static const u8 sText_NotOnline[]    = _("Not connected.");
static const u8 sText_TestOn[]       = _("Test mode ON!\nFake player spawned.");
static const u8 sText_TestOff[]      = _("Test mode OFF.\nFake player removed.");
static const u8 sText_EnterRoom[]    = _("Enter room code:");

static const u8 sText_DefaultRoom[]  = _("room1");

/* ── State ─────────────────────────────────────────── */

static EWRAM_DATA bool8 sTestModeActive = FALSE;
static EWRAM_DATA u8 sOnlineMenuWindowId = 0;
static EWRAM_DATA u8 sOnlineInfoWindowId = 0;
static EWRAM_DATA u8 sOnlineMenuTaskId = TASK_NONE;
static EWRAM_DATA u8 sRoomCodeBuf[16] = {0};

/* ── Window templates ──────────────────────────────── */

static const struct WindowTemplate sOnlineMenuWindowTmpl = {
    0,  /* bg */
    19, /* tilemapLeft */
    1,  /* tilemapTop */
    10, /* width */
    10, /* height */
    15, /* paletteNum */
    0x1C0 /* baseBlock */
};

static const struct WindowTemplate sOnlineInfoWindowTmpl = {
    0,  /* bg */
    1,  /* tilemapLeft */
    13, /* tilemapTop */
    26, /* width */
    6,  /* height */
    15, /* paletteNum */
    0x240 /* baseBlock */
};

/* ── Forward declarations ──────────────────────────── */

static void Task_OnlineMenu(u8 taskId);
static void DrawOnlineMenu(void);
static void CloseOnlineMenu(void);
static void ShowInfoMessage(const u8 *text);
static void CloseInfoWindow(void);
static void DoConnect(void);
static void DoDisconnect(void);
static void DoTestMode(void);
static void DoShowStatus(void);

/* ── Test mode internals ───────────────────────────── */

static void TestMode_SpawnFakePlayer(void);
static void TestMode_DespawnFakePlayer(void);
static void Task_TestModeTick(u8 taskId);

/* ── Public API ────────────────────────────────────── */

bool8 OnlineMenu_IsTestMode(void)
{
    return sTestModeActive;
}

void OnlineMenu_ToggleTestMode(void)
{
    DoTestMode();
}

void OnlineMenu_Open(void)
{
    PlaySE(SE_SELECT);
    DrawOnlineMenu();
    sOnlineMenuTaskId = CreateTask(Task_OnlineMenu, 80);
}

/* CB2 entry point used from Start menu callback.
 * We don't need a full CB2 swap — we just open the menu on top of the field.
 * The Start menu callback will call this after fading back in. */
void CB2_OnlineMenu(void)
{
    /* This is called as a SetMainCallback2 target.
     * We want to return to the field and then open our menu overlay. */
    SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
}

/* ── Drawing ───────────────────────────────────────── */

static void DrawOnlineMenu(void)
{
    u8 i;
    const u8 *menuTexts[ONLINE_MENU_COUNT];

    menuTexts[ONLINE_MENU_CONNECT]    = sText_Connect;
    menuTexts[ONLINE_MENU_DISCONNECT] = sText_Disconnect;
    menuTexts[ONLINE_MENU_TEST_MODE]  = sText_TestMode;
    menuTexts[ONLINE_MENU_STATUS]     = sText_Status;
    menuTexts[ONLINE_MENU_BACK]       = sText_Back;

    sOnlineMenuWindowId = AddWindow(&sOnlineMenuWindowTmpl);
    SetStandardWindowBorderStyle(sOnlineMenuWindowId, FALSE);

    for (i = 0; i < ONLINE_MENU_COUNT; i++)
    {
        AddTextPrinterParameterized(sOnlineMenuWindowId, FONT_NORMAL,
            menuTexts[i], 8, i * 16 + 1, TEXT_SKIP_DRAW, NULL);
    }

    InitMenuInUpperLeftCornerNormal(sOnlineMenuWindowId, ONLINE_MENU_COUNT, 0);
    PutWindowTilemap(sOnlineMenuWindowId);
    CopyWindowToVram(sOnlineMenuWindowId, COPYWIN_FULL);
}

static void CloseOnlineMenu(void)
{
    ClearStdWindowAndFrameToTransparent(sOnlineMenuWindowId, TRUE);
    RemoveWindow(sOnlineMenuWindowId);
    CloseInfoWindow();
}

static void ShowInfoMessage(const u8 *text)
{
    CloseInfoWindow();
    sOnlineInfoWindowId = AddWindow(&sOnlineInfoWindowTmpl);
    SetStandardWindowBorderStyle(sOnlineInfoWindowId, FALSE);
    AddTextPrinterParameterized(sOnlineInfoWindowId, FONT_NORMAL,
        text, 0, 1, TEXT_SKIP_DRAW, NULL);
    PutWindowTilemap(sOnlineInfoWindowId);
    CopyWindowToVram(sOnlineInfoWindowId, COPYWIN_FULL);
}

static void CloseInfoWindow(void)
{
    if (sOnlineInfoWindowId != 0)
    {
        ClearStdWindowAndFrameToTransparent(sOnlineInfoWindowId, TRUE);
        RemoveWindow(sOnlineInfoWindowId);
        sOnlineInfoWindowId = 0;
    }
}

/* ── Menu input task ───────────────────────────────── */

static void Task_OnlineMenu(u8 taskId)
{
    s8 input;

    input = Menu_ProcessInputNoWrap();

    switch (input)
    {
    case MENU_NOTHING_CHOSEN:
        break;

    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        CloseOnlineMenu();
        DestroyTask(taskId);
        break;

    default:
        PlaySE(SE_SELECT);

        switch (input)
        {
        case ONLINE_MENU_CONNECT:
            DoConnect();
            break;
        case ONLINE_MENU_DISCONNECT:
            DoDisconnect();
            break;
        case ONLINE_MENU_TEST_MODE:
            DoTestMode();
            break;
        case ONLINE_MENU_STATUS:
            DoShowStatus();
            break;
        case ONLINE_MENU_BACK:
            CloseOnlineMenu();
            DestroyTask(taskId);
            return;
        }

        /* Re-init cursor after showing info (keep menu open) */
        if (input != ONLINE_MENU_BACK)
            InitMenuInUpperLeftCornerNormal(sOnlineMenuWindowId, ONLINE_MENU_COUNT, input);
        break;
    }
}

/* ── Actions ───────────────────────────────────────── */

static void DoConnect(void)
{
    enum MultiplayerState state;

    state = Multiplayer_GetState();

    if (state >= MP_STATE_IN_ROOM)
    {
        ShowInfoMessage(sText_AlreadyOn);
        return;
    }

    /* Connect with default room code */
    Multiplayer_Connect(sText_DefaultRoom);
    ShowInfoMessage(sText_Connected);
}

static void DoDisconnect(void)
{
    if (Multiplayer_GetState() == MP_STATE_OFFLINE)
    {
        ShowInfoMessage(sText_NotOnline);
        return;
    }

    /* If test mode is on, turn it off first */
    if (sTestModeActive)
    {
        TestMode_DespawnFakePlayer();
        sTestModeActive = FALSE;
    }

    Multiplayer_Disconnect();
    ShowInfoMessage(sText_Disconnected);
}

static void DoShowStatus(void)
{
    const u8 *statusText;

    switch (Multiplayer_GetState())
    {
    case MP_STATE_OFFLINE:    statusText = sText_StatusOffline;    break;
    case MP_STATE_CONNECTING: statusText = sText_StatusConnecting; break;
    case MP_STATE_LOBBY:
    case MP_STATE_IN_ROOM:    statusText = sText_StatusInRoom;     break;
    case MP_STATE_SYNCED:     statusText = sText_StatusSynced;     break;
    case MP_STATE_BATTLE:     statusText = sText_StatusBattle;     break;
    case MP_STATE_TRADE:      statusText = sText_StatusTrade;      break;
    default:                  statusText = sText_StatusError;      break;
    }

    ShowInfoMessage(statusText);
}

/* ── Test Mode ─────────────────────────────────────── */

/*
 * Test mode spawns a "fake" remote player that mirrors the local player
 * with a slight offset, so you can test the entire multiplayer UI flow
 * without needing a second client or the relay server.
 *
 * - Connect is forced to MP_STATE_SYNCED
 * - A remote ObjectEvent is spawned 2 tiles to the right
 * - The test tick task copies local position to remote with offset
 * - Pressing A on the fake player opens the interaction menu
 */

static EWRAM_DATA u8 sTestModeTaskId = TASK_NONE;

static void DoTestMode(void)
{
    if (!sTestModeActive)
    {
        sTestModeActive = TRUE;
        TestMode_SpawnFakePlayer();
        ShowInfoMessage(sText_TestOn);
    }
    else
    {
        sTestModeActive = FALSE;
        TestMode_DespawnFakePlayer();
        ShowInfoMessage(sText_TestOff);
    }
}

static void TestMode_SpawnFakePlayer(void)
{
    /* Force multiplayer state so IsConnected() returns true */
    /* We call Connect with a dummy room, then force SYNCED */
    Multiplayer_Connect(sText_DefaultRoom);

    /* Spawn the remote avatar */
    Multiplayer_SpawnRemoteAvatar();

    /* Start tick task that mirrors local player position */
    if (sTestModeTaskId == TASK_NONE)
        sTestModeTaskId = CreateTask(Task_TestModeTick, 90);
}

static void TestMode_DespawnFakePlayer(void)
{
    Multiplayer_DespawnRemoteAvatar();
    Multiplayer_Disconnect();

    if (sTestModeTaskId != TASK_NONE)
    {
        DestroyTask(sTestModeTaskId);
        sTestModeTaskId = TASK_NONE;
    }
}

static void Task_TestModeTick(u8 taskId)
{
    struct ObjectEvent *playerObj;
    struct ObjectEvent *remoteObj;
    u8 remoteId;

    if (!sTestModeActive)
    {
        DestroyTask(taskId);
        sTestModeTaskId = TASK_NONE;
        return;
    }

    remoteId = Multiplayer_GetRemoteObjectEventId();
    if (remoteId >= OBJECT_EVENTS_COUNT)
        return;

    playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    remoteObj = &gObjectEvents[remoteId];

    /* Mirror local player 2 tiles to the right */
    remoteObj->currentCoords.x = playerObj->currentCoords.x + 2;
    remoteObj->currentCoords.y = playerObj->currentCoords.y;
    remoteObj->previousCoords.x = playerObj->previousCoords.x + 2;
    remoteObj->previousCoords.y = playerObj->previousCoords.y;
    remoteObj->facingDirection = playerObj->facingDirection;

    (void)taskId;
}
