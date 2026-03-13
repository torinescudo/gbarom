#include "global.h"
#include "ui_textbox.h"
#include "ui_theme.h"
#include "ui_window_frame.h"

/*
 * Modernised textbox — LC theme.
 *
 * Wraps pokeemerald's text printer with:
 *   - 3-line display area.
 *   - Drop shadow per glyph.
 *   - Animated "advance" indicator.
 *   - Smooth character reveal at configurable speed.
 *
 * Integration points (replace in pokeemerald code):
 *   DrawDialogueFrame → UiTextbox_Open
 *   AddTextPrinterParameterized → UiTextbox_Print
 *   Task_DrawFieldMessage loop → call UiTextbox_TickAdvanceIndicator
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

// ── Private helpers ────────────────────────────────────────

static struct TextboxState *GetState(u8 windowId)
{
    if (windowId >= MAX_TEXTBOX_WINDOWS)
        return NULL;
    return &sTextboxes[windowId];
}

// ── Public API ─────────────────────────────────────────────

void UiTextbox_Init(struct UiTextboxConfig *cfg)
{
    struct TextboxState *st;

    if (cfg == NULL || cfg->windowId >= MAX_TEXTBOX_WINDOWS)
        return;

    st = &sTextboxes[cfg->windowId];
    st->active           = FALSE;
    st->waitingForInput   = FALSE;
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
        // Draw LC dialog frame around window.
        UiWindowFrame_Draw(windowId, FRAME_STYLE_DIALOG);

        // Apply dialog palette.
        UiTheme_ApplyDialogPalette();

        // In real build:
        //   PutWindowTilemap(windowId);
        //   CopyWindowToVram(windowId, COPYWIN_FULL);
    }
    else
    {
        // Vanilla:
        //   DrawDialogueFrame(windowId, FALSE);
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

    // In real build:
    //   ClearDialogWindowAndFrame(windowId, TRUE);
}

void UiTextbox_Print(u8 windowId, const u8 *text)
{
    struct TextboxState *st = GetState(windowId);

    if (st == NULL || !st->active || text == NULL)
        return;

    // In real integration this sets up the text printer:
    //
    //   struct TextPrinterTemplate template = {
    //       .currentChar = text,
    //       .windowId    = windowId,
    //       .fontId      = FONT_NORMAL,
    //       .x           = st->padSide,
    //       .y           = st->padTop,
    //       .fgColor     = 4,   // LC_TEXT_PRIMARY
    //       .bgColor     = 1,   // LC_DIALOG_BG or LC_BG_DARK
    //       .shadowColor = 11,  // LC_SHADOW
    //       .letterSpacing = 0,
    //       .lineSpacing   = 2,
    //       .speed         = st->textSpeed,
    //   };
    //   AddTextPrinter(&template, st->textSpeed, NULL);
    //
    // For shadow: pokeemerald already supports shadow color in the printer,
    // so we just set it to LC_SHADOW (pal index 11) and enable it.

    (void)text;
}

void UiTextbox_TickAdvanceIndicator(u8 windowId)
{
    struct TextboxState *st = GetState(windowId);
    if (st == NULL || !st->active)
        return;

    // Show bouncing triangle only when waiting for player input.
    if (!st->waitingForInput)
    {
        st->advanceOffsetY = 0;
        return;
    }

    st->advanceAnimTimer++;
    u16 period = LC_CURSOR_BLINK_PERIOD * 2;
    u16 phase  = st->advanceAnimTimer % period;

    if (phase < LC_CURSOR_BLINK_PERIOD)
        st->advanceOffsetY = (s8)((phase * LC_CURSOR_BOUNCE_PX) / LC_CURSOR_BLINK_PERIOD);
    else
        st->advanceOffsetY = (s8)(LC_CURSOR_BOUNCE_PX
            - ((phase - LC_CURSOR_BLINK_PERIOD) * LC_CURSOR_BOUNCE_PX) / LC_CURSOR_BLINK_PERIOD);

    // In real build, reposition the advance arrow sprite:
    //   gSprites[advanceSpriteId].y2 = st->advanceOffsetY;
}

bool8 UiTextbox_IsAnimating(u8 windowId)
{
    struct TextboxState *st = GetState(windowId);
    if (st == NULL)
        return FALSE;

    // In real build, query the text printer:
    //   return IsTextPrinterActive(windowId);
    return FALSE;
}

void UiTextbox_SetSpeed(u8 windowId, u8 charsPerFrame)
{
    struct TextboxState *st = GetState(windowId);
    if (st == NULL)
        return;

    st->textSpeed = (charsPerFrame > 0) ? charsPerFrame : 1;
}
