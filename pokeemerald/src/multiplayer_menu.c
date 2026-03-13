#include "global.h"
#include "multiplayer.h"
#include "multiplayer_menu.h"
#include "task.h"
#include "menu.h"
#include "window.h"
#include "string_util.h"
#include "text.h"
#include "sound.h"
#include "constants/songs.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"

/*
 * multiplayer_menu.c - Interaction menu for on-the-spot multiplayer.
 */

/* Script bytecode: releaseall (0x6b) + end (0x02) */
const u8 gMultiplayerInteractScript[] = { 0x6b, 0x02 };

/* Menu items */
enum {
    MENU_ITEM_BATTLE = 0,
    MENU_ITEM_TRADE,
    MENU_ITEM_CARD,
    MENU_ITEM_CANCEL,
    MENU_ITEM_COUNT
};

static const u8 sText_Battle[]  = _("Battle");
static const u8 sText_Trade[]   = _("Trade");
static const u8 sText_Card[]    = _("Player Card");
static const u8 sText_Cancel[]  = _("Cancel");
static const u8 sText_WantsToBattle[] = _("{STR_VAR_1} wants to battle!\nAccept?");
static const u8 sText_WantsToTrade[]  = _("{STR_VAR_1} wants to trade!\nAccept?");
static const u8 sText_RemotePlayer[] = _("Player");

/* Window templates */
static const struct WindowTemplate sMenuWindowTemplate = {
    0, /* bg */
    22, /* tilemapLeft */
    1,  /* tilemapTop */
    7,  /* width */
    8,  /* height */
    15, /* paletteNum */
    0x1C0 /* baseBlock */
};

static const struct WindowTemplate sPromptWindowTemplate = {
    0, /* bg */
    1, /* tilemapLeft */
    15, /* tilemapTop */
    28, /* width */
    4,  /* height */
    15, /* paletteNum */
    0x200 /* baseBlock */
};

static const struct WindowTemplate sYesNoWindowTemplate = {
    0, /* bg */
    24, /* tilemapLeft */
    9,  /* tilemapTop */
    5,  /* width */
    4,  /* height */
    15, /* paletteNum */
    0x260 /* baseBlock */
};

/* State */
static EWRAM_DATA bool8 sMenuOpen = FALSE;
static EWRAM_DATA u8 sMenuWindowId = 0;
static EWRAM_DATA u8 sMenuTaskId = TASK_NONE;
static EWRAM_DATA u8 sPromptWindowId = 0;

/* Forward declarations */
static void Task_MultiplayerMenu(u8 taskId);
static void Task_IncomingPrompt(u8 taskId);
static void OpenMenuWindow(void);
static void CloseMenuWindow(void);

/* Public API */

bool8 MultiplayerMenu_CheckAndOpenOnInteraction(u8 objectEventId)
{
    if (!Multiplayer_IsConnected())
        return FALSE;

    if (objectEventId >= OBJECT_EVENTS_COUNT)
        return FALSE;

    if (gObjectEvents[objectEventId].localId != LOCALID_REMOTE_PLAYER)
        return FALSE;

    MultiplayerMenu_Open();
    return TRUE;
}

void MultiplayerMenu_Open(void)
{
    if (sMenuOpen)
        return;

    sMenuOpen = TRUE;
    PlaySE(SE_SELECT);
    OpenMenuWindow();
    sMenuTaskId = CreateTask(Task_MultiplayerMenu, 80);
}

bool8 MultiplayerMenu_IsOpen(void)
{
    return sMenuOpen;
}

void MultiplayerMenu_HandleIncomingRequest(void)
{
    if (!Multiplayer_HasPendingInteraction())
        return;

    if (sMenuOpen)
        return;

    CreateTask(Task_IncomingPrompt, 80);
}

/* Menu window */

static void OpenMenuWindow(void)
{
    u8 i;

    sMenuWindowId = AddWindow(&sMenuWindowTemplate);
    SetStandardWindowBorderStyle(sMenuWindowId, FALSE);

    for (i = 0; i < MENU_ITEM_COUNT; i++)
    {
        const u8 *text;
        switch (i)
        {
        case MENU_ITEM_BATTLE: text = sText_Battle; break;
        case MENU_ITEM_TRADE:  text = sText_Trade;  break;
        case MENU_ITEM_CARD:   text = sText_Card;   break;
        default:               text = sText_Cancel;  break;
        }
        AddTextPrinterParameterized(sMenuWindowId, FONT_NORMAL,
            text, 8, i * 16 + 1, TEXT_SKIP_DRAW, NULL);
    }

    InitMenuInUpperLeftCornerNormal(sMenuWindowId, MENU_ITEM_COUNT, 0);
    PutWindowTilemap(sMenuWindowId);
    CopyWindowToVram(sMenuWindowId, COPYWIN_FULL);
}

static void CloseMenuWindow(void)
{
    ClearStdWindowAndFrameToTransparent(sMenuWindowId, TRUE);
    RemoveWindow(sMenuWindowId);
    sMenuOpen = FALSE;
}

/* Menu task */

static void Task_MultiplayerMenu(u8 taskId)
{
    s8 input;

    input = Menu_ProcessInputNoWrap();

    switch (input)
    {
    case MENU_NOTHING_CHOSEN:
        break;

    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        CloseMenuWindow();
        DestroyTask(taskId);
        break;

    default:
        PlaySE(SE_SELECT);
        CloseMenuWindow();

        switch (input)
        {
        case MENU_ITEM_BATTLE:
            Multiplayer_RequestInteraction(MP_INTERACT_BATTLE);
            break;
        case MENU_ITEM_TRADE:
            Multiplayer_RequestInteraction(MP_INTERACT_TRADE);
            break;
        case MENU_ITEM_CARD:
            Multiplayer_RequestInteraction(MP_INTERACT_SHOW_CARD);
            break;
        case MENU_ITEM_CANCEL:
        default:
            break;
        }
        DestroyTask(taskId);
        break;
    }
}

/* Incoming interaction prompt */

static void Task_IncomingPrompt(u8 taskId)
{
    s16 *data;
    enum MultiplayerInteraction req;
    const u8 *msg;
    s8 result;

    data = gTasks[taskId].data;

    switch (data[0])
    {
    case 0:
        req = Multiplayer_GetPendingInteraction();

        if (req == MP_INTERACT_BATTLE)
            msg = sText_WantsToBattle;
        else if (req == MP_INTERACT_TRADE)
            msg = sText_WantsToTrade;
        else
        {
            Multiplayer_DeclineInteraction();
            DestroyTask(taskId);
            return;
        }

        sPromptWindowId = AddWindow(&sPromptWindowTemplate);
        SetStandardWindowBorderStyle(sPromptWindowId, FALSE);

        StringCopy(gStringVar1, sText_RemotePlayer);
        StringExpandPlaceholders(gStringVar4, msg);
        AddTextPrinterParameterized(sPromptWindowId, FONT_NORMAL,
            gStringVar4, 0, 1, TEXT_SKIP_DRAW, NULL);
        PutWindowTilemap(sPromptWindowId);
        CopyWindowToVram(sPromptWindowId, COPYWIN_FULL);

        CreateYesNoMenu(&sYesNoWindowTemplate, 0x001, 14, 0);

        data[0] = 1;
        break;

    case 1:
        result = Menu_ProcessInputNoWrapClearOnChoose();

        if (result == 0)
        {
            PlaySE(SE_SELECT);
            ClearStdWindowAndFrameToTransparent(sPromptWindowId, TRUE);
            RemoveWindow(sPromptWindowId);
            Multiplayer_AcceptInteraction();
            DestroyTask(taskId);
        }
        else if (result == 1 || result == MENU_B_PRESSED)
        {
            PlaySE(SE_SELECT);
            ClearStdWindowAndFrameToTransparent(sPromptWindowId, TRUE);
            RemoveWindow(sPromptWindowId);
            Multiplayer_DeclineInteraction();
            DestroyTask(taskId);
        }
        break;
    }
}
