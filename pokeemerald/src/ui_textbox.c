#include "global.h"
#include "ui_textbox.h"
#include "ui_theme.h"
#include "ui_window_frame.h"

/*
 * Modernised textbox — LC theme (C89 compatible).
 */

#define MAX_TEXTBOX_WINDOWS 4

struct TextboxState
{
    bool8 active;
    bool8 waitingForInput;
    u8    textSpeed;
    u8    padTop;
    u8    padSide;
    u16   advanceAnimTimer;
    s8    advanceOffsetY;
};

static struct TextboxState sTextboxes[MAX_TEXTBOX_WINDOWS];

static struct TextboxState *GetState(u8 windowId)
{
    if (windowId >= MAX_TEXTBOX_WINDOWS)
        return NULL;
    return &sTextboxes[windowId];
}

void UiTextbox_Init(struct UiTextboxConfig *cfg)
{
    struct TextboxState *st;

    if (cfg == NULL || cfg->windowId >= MAX_TEXTBOX_WINDOWS)
        return;

    st = &sTextboxes[cfg->windowId];
    st->active           = FALSE;
    st->waitingForInput  = FALSE;
    st->textSpeed        = cfg->textSpeed ? cfg->textSpeed : LC_TEXT_SPEED_DEFAULT;
    st->padTop           = cfg->padTop    ? cfg->padTop    : LC_TEXTBOX_PAD_TOP;
    st->padSide          = cfg->padSide   ? cfg->padSide   : LC_TEXTBOX_PAD_SIDE;
    st->advanceAnimTimer = 0;
    st->advanceOffsetY   = 0;
}

void UiTextbox_Open(u8 windowId)
{
    struct TextboxState *st = GetState(windowId);
    if (st == NULL)
        return;

    st->active = TRUE;
    st->waitingForInput = FALSE;

    if (UiTheme_GetCurrent() == UI_THEME_LIQUID_CRYSTAL)
    {
        UiWindowFrame_Draw(windowId, FRAME_STYLE_DIALOG);
        UiTheme_ApplyDialogPalette();
    }
}

void UiTextbox_Close(u8 windowId)
{
    struct TextboxState *st = GetState(windowId);
    if (st == NULL)
        return;

    st->active = FALSE;
    st->waitingForInput = FALSE;
    UiWindowFrame_Clear(windowId);
}

void UiTextbox_Print(u8 windowId, const u8 *text)
{
    struct TextboxState *st = GetState(windowId);

    if (st == NULL || !st->active || text == NULL)
        return;

    (void)text;
}

void UiTextbox_TickAdvanceIndicator(u8 windowId)
{
    struct TextboxState *st = GetState(windowId);
    u16 period;
    u16 phase;

    if (st == NULL || !st->active)
        return;

    if (!st->waitingForInput)
    {
        st->advanceOffsetY = 0;
        return;
    }

    st->advanceAnimTimer++;
    period = LC_CURSOR_BLINK_PERIOD * 2;
    phase  = st->advanceAnimTimer % period;

    if (phase < LC_CURSOR_BLINK_PERIOD)
        st->advanceOffsetY = (s8)((phase * LC_CURSOR_BOUNCE_PX) / LC_CURSOR_BLINK_PERIOD);
    else
        st->advanceOffsetY = (s8)(LC_CURSOR_BOUNCE_PX
            - ((phase - LC_CURSOR_BLINK_PERIOD) * LC_CURSOR_BOUNCE_PX) / LC_CURSOR_BLINK_PERIOD);
}

bool8 UiTextbox_IsAnimating(u8 windowId)
{
    struct TextboxState *st = GetState(windowId);
    if (st == NULL)
        return FALSE;
    return FALSE;
}

void UiTextbox_SetSpeed(u8 windowId, u8 charsPerFrame)
{
    struct TextboxState *st = GetState(windowId);
    if (st == NULL)
        return;
    st->textSpeed = (charsPerFrame > 0) ? charsPerFrame : 1;
}
