#include "global.h"
#include "ui_theme.h"
#include "palette.h"
#include "gpu_regs.h"

/*
 * UI Theme — Liquid Crystal style implementation.
 * All code is C89-compatible for agbcc.
 */

static EWRAM_DATA enum UiThemeId sCurrentTheme = UI_THEME_VANILLA;
static EWRAM_DATA u8  sTextSpeed = 1;
static EWRAM_DATA bool8 sFading = FALSE;
static EWRAM_DATA s8  sFadeStep = 0;
static EWRAM_DATA u8  sFadeCounter = 0;
static EWRAM_DATA u8  sFadeLevel = 0;
static EWRAM_DATA u16 sCursorAnimTimer = 0;
static EWRAM_DATA s8  sCursorOffsetY = 0;

static const u16 sLcMenuPalette[16] =
{
    LC_TRANSPARENT,
    LC_BG_DARK,
    LC_BG_MID,
    LC_BORDER_OUTER,
    LC_TEXT_PRIMARY,
    LC_ACCENT_CYAN,
    LC_ACCENT_GOLD,
    LC_COLOR_RED,
    LC_COLOR_GREEN,
    LC_COLOR_BLUE,
    LC_TEXT_SECONDARY,
    LC_SHADOW,
    0, 0, 0, 0,
};

static const u16 sLcDialogPalette[16] =
{
    LC_TRANSPARENT,
    LC_DIALOG_BG,
    LC_BG_MID,
    LC_BORDER_OUTER,
    LC_TEXT_PRIMARY,
    LC_ACCENT_CYAN,
    LC_ACCENT_GOLD,
    LC_COLOR_RED,
    LC_COLOR_GREEN,
    LC_COLOR_BLUE,
    LC_TEXT_SECONDARY,
    LC_SHADOW,
    0, 0, 0, 0,
};

static const u16 sLcCursorPalette[16] =
{
    LC_TRANSPARENT,
    LC_CURSOR_BRIGHT,
    LC_CURSOR_DIM,
    LC_TEXT_PRIMARY,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};

/* Pixel helper: set 4bpp pixel in 8x8 tile buffer */
static void SetTilePixel(u8 *tile, u8 x, u8 y, u8 palIdx)
{
    u32 offset = y * 4 + (x / 2);
    if (x & 1)
        tile[offset] = (tile[offset] & 0x0F) | (palIdx << 4);
    else
        tile[offset] = (tile[offset] & 0xF0) | (palIdx & 0x0F);
}

static void FillTile(u8 *tile, u8 palIdx)
{
    u8 x, y;
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++)
            SetTilePixel(tile, x, y, palIdx);
}

static void GenerateFrameTile(u8 *tile, u8 pos)
{
    u8 x, y;
    bool8 isTop    = (pos < 3);
    bool8 isBottom = (pos >= 6);
    bool8 isLeft   = (pos % 3 == 0);
    bool8 isRight  = (pos % 3 == 2);

    FillTile(tile, 1);

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            if ((isTop && y == 0) || (isBottom && y == 7)
                || (isLeft && x == 0) || (isRight && x == 7))
            {
                SetTilePixel(tile, x, y, 3);
            }

            if ((isTop && y == 1 && x > 0 && (!isRight || x < 7))
                || (isBottom && y == 6 && x > 0 && (!isRight || x < 7))
                || (isLeft && x == 1 && y > 0 && (!isBottom || y < 7))
                || (isRight && x == 6 && y > 0 && (!isBottom || y < 7)))
            {
                SetTilePixel(tile, x, y, 5);
            }

            if ((isBottom && y == 7 && x > 0) || (isRight && x == 7 && y > 0))
            {
                SetTilePixel(tile, x, y, 11);
            }

            if (isTop && isLeft && x < 2 && y < 2 && (x + y < 2))
                SetTilePixel(tile, x, y, 0);
            if (isTop && isRight && x > 5 && y < 2 && ((7 - x) + y < 2))
                SetTilePixel(tile, x, y, 0);
            if (isBottom && isLeft && x < 2 && y > 5 && (x + (7 - y) < 2))
                SetTilePixel(tile, x, y, 0);
            if (isBottom && isRight && x > 5 && y > 5 && ((7 - x) + (7 - y) < 2))
                SetTilePixel(tile, x, y, 0);
        }
    }
}

static u8 sFrameTileBuffer[WIN_FRAME_TILE_COUNT * 32];

/* ── Public API ───────────────────────────────────────── */

void UiTheme_Init(void)
{
    sCurrentTheme    = UI_THEME_LIQUID_CRYSTAL;
    sTextSpeed       = LC_TEXT_SPEED_DEFAULT;
    sFading          = FALSE;
    sFadeStep        = 0;
    sFadeCounter     = 0;
    sFadeLevel       = 0;
    sCursorAnimTimer = 0;
    sCursorOffsetY   = 0;

    UiTheme_ApplyAllPalettes();
}

void UiTheme_SetTheme(enum UiThemeId themeId)
{
    if (themeId >= UI_THEME_COUNT)
        return;
    sCurrentTheme = themeId;
    UiTheme_ApplyAllPalettes();
}

enum UiThemeId UiTheme_GetCurrent(void)
{
    return sCurrentTheme;
}

void UiTheme_ApplyMenuPalette(void)
{
    if (sCurrentTheme == UI_THEME_LIQUID_CRYSTAL)
        LoadPalette(sLcMenuPalette, LC_PAL_SLOT_MENU_BG * 16, 32);
}

void UiTheme_ApplyDialogPalette(void)
{
    if (sCurrentTheme == UI_THEME_LIQUID_CRYSTAL)
        LoadPalette(sLcDialogPalette, LC_PAL_SLOT_DIALOG_BG * 16, 32);
}

void UiTheme_ApplyCursorPalette(void)
{
    if (sCurrentTheme == UI_THEME_LIQUID_CRYSTAL)
        LoadPalette(sLcCursorPalette, 256 + LC_PAL_SLOT_CURSOR * 16, 32);
}

void UiTheme_ApplyAllPalettes(void)
{
    UiTheme_ApplyMenuPalette();
    UiTheme_ApplyDialogPalette();
    UiTheme_ApplyCursorPalette();
}

void UiTheme_LoadWindowFrameTiles(void)
{
    u8 i;

    if (sCurrentTheme != UI_THEME_LIQUID_CRYSTAL)
        return;

    for (i = 0; i < WIN_FRAME_TILE_COUNT; i++)
        GenerateFrameTile(&sFrameTileBuffer[i * 32], i);
}

void UiTheme_DrawHighlightBar(u8 y, u8 height)
{
    if (sCurrentTheme != UI_THEME_LIQUID_CRYSTAL)
        return;
    (void)y;
    (void)height;
}

void UiTheme_SetupFadeIn(void)
{
    sFading      = TRUE;
    sFadeStep    = -1;
    sFadeLevel   = 16;
    sFadeCounter = 0;
}

void UiTheme_SetupFadeOut(void)
{
    sFading      = TRUE;
    sFadeStep    = 1;
    sFadeLevel   = 0;
    sFadeCounter = 0;
}

void UiTheme_TickFade(void)
{
    if (!sFading)
        return;

    sFadeCounter++;
    if (sFadeCounter < (16 / LC_FADE_FRAMES))
        return;

    sFadeCounter = 0;
    sFadeLevel = (u8)((s8)sFadeLevel + sFadeStep);

    if ((sFadeStep > 0 && sFadeLevel >= 16) || (sFadeStep < 0 && sFadeLevel == 0))
        sFading = FALSE;
}

bool8 UiTheme_IsFading(void)
{
    return sFading;
}

void UiTheme_SetTextSpeed(u8 charsPerFrame)
{
    sTextSpeed = (charsPerFrame > 0) ? charsPerFrame : 1;
}

u8 UiTheme_GetTextSpeed(void)
{
    return sTextSpeed;
}

void UiTheme_TickCursorAnim(void)
{
    u16 phase;

    sCursorAnimTimer++;
    phase = sCursorAnimTimer % (LC_CURSOR_BLINK_PERIOD * 2);

    if (phase < LC_CURSOR_BLINK_PERIOD)
        sCursorOffsetY = (s8)((phase * LC_CURSOR_BOUNCE_PX) / LC_CURSOR_BLINK_PERIOD);
    else
        sCursorOffsetY = (s8)(LC_CURSOR_BOUNCE_PX - ((phase - LC_CURSOR_BLINK_PERIOD) * LC_CURSOR_BOUNCE_PX) / LC_CURSOR_BLINK_PERIOD);
}

s8 UiTheme_GetCursorOffsetY(void)
{
    return sCursorOffsetY;
}
