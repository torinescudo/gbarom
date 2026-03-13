#include "global.h"
#include "ui_window_frame.h"
#include "ui_theme.h"

/*
 * Window frame drawing — LC theme.
 *
 * Generates and draws 3×3 tile-based frames with:
 *   - 1px outer border
 *   - 1px inner glow (cyan)
 *   - 1px drop shadow (bottom-right)
 *   - Rounded corners (2px radius)
 *   - Dark solid fill
 *
 * Integration: replace calls to DrawStdWindowFrame /
 * DrawDialogFrameWithCustomTileAndPalette in pokeemerald
 * with UiWindowFrame_Draw(windowId, style).
 */

// ── Internal tile cache ────────────────────────────────────

#define TILE_BYTES_4BPP 32  // 8×8 @ 4bpp

// We keep one generated set per style.
static u8 sStyleTiles[3][WIN_FRAME_TILE_COUNT * TILE_BYTES_4BPP];
static bool8 sStyleGenerated[3];

// ── Pixel-level helpers (same as in ui_theme.c) ────────────

static void WF_SetPixel(u8 *tile, u8 x, u8 y, u8 palIdx)
{
    u32 off = y * 4 + (x / 2);
    if (x & 1)
        tile[off] = (tile[off] & 0x0F) | (palIdx << 4);
    else
        tile[off] = (tile[off] & 0xF0) | (palIdx & 0x0F);
}

static void WF_FillTile(u8 *tile, u8 palIdx)
{
    u8 x, y;
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++)
            WF_SetPixel(tile, x, y, palIdx);
}

// Palette indices per style.
static u8 GetBgIndex(u8 style)
{
    // 1 = LC_BG_DARK for standard/battle, slightly lighter for dialog.
    return (style == FRAME_STYLE_DIALOG) ? 2 : 1;
}

static u8 GetBorderIndex(u8 style)
{
    (void)style;
    return 3; // LC_BORDER_OUTER
}

static u8 GetGlowIndex(u8 style)
{
    // Battle frames use gold glow instead of cyan.
    return (style == FRAME_STYLE_BATTLE) ? 6 : 5;
}

static u8 GetShadowIndex(u8 style)
{
    (void)style;
    return 11; // LC_SHADOW
}

static void GenerateStyledTile(u8 *tile, u8 pos, u8 style)
{
    u8 x, y;
    u8 bgIdx     = GetBgIndex(style);
    u8 borderIdx = GetBorderIndex(style);
    u8 glowIdx   = GetGlowIndex(style);
    u8 shadowIdx = GetShadowIndex(style);

    bool8 isTop    = (pos < 3);
    bool8 isBottom = (pos >= 6);
    bool8 isLeft   = (pos % 3 == 0);
    bool8 isRight  = (pos % 3 == 2);

    WF_FillTile(tile, bgIdx);

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            // Outer 1px border
            if ((isTop && y == 0) || (isBottom && y == 7)
                || (isLeft && x == 0) || (isRight && x == 7))
            {
                WF_SetPixel(tile, x, y, borderIdx);
            }

            // Inner glow
            if ((isTop && y == 1 && !(isLeft && x == 0) && !(isRight && x == 7))
                || (isBottom && y == 6 && !(isLeft && x == 0) && !(isRight && x == 7))
                || (isLeft && x == 1 && !(isTop && y == 0) && !(isBottom && y == 7))
                || (isRight && x == 6 && !(isTop && y == 0) && !(isBottom && y == 7)))
            {
                WF_SetPixel(tile, x, y, glowIdx);
            }

            // Shadow on outer edge
            if ((isBottom && y == 7 && x > 0) || (isRight && x == 7 && y > 0))
            {
                WF_SetPixel(tile, x, y, shadowIdx);
            }

            // Rounded corners: 2px radius (clear outermost 1+1 diagonal).
            if (isTop && isLeft && x == 0 && y == 0)
                WF_SetPixel(tile, x, y, 0);
            if (isTop && isRight && x == 7 && y == 0)
                WF_SetPixel(tile, x, y, 0);
            if (isBottom && isLeft && x == 0 && y == 7)
                WF_SetPixel(tile, x, y, 0);
            if (isBottom && isRight && x == 7 && y == 7)
                WF_SetPixel(tile, x, y, 0);
        }
    }
}

// ── Public ─────────────────────────────────────────────────

void UiWindowFrame_GenerateTiles(u8 style)
{
    u8 i;

    if (style > FRAME_STYLE_BATTLE)
        return;

    for (i = 0; i < WIN_FRAME_TILE_COUNT; i++)
        GenerateStyledTile(&sStyleTiles[style][i * TILE_BYTES_4BPP], i, style);

    sStyleGenerated[style] = TRUE;
}

void UiWindowFrame_Draw(u8 windowId, u8 style)
{
    if (UiTheme_GetCurrent() != UI_THEME_LIQUID_CRYSTAL)
    {
        // Fall through to vanilla frame drawing.
        // In real integration:
        //   DrawStdWindowFrame(windowId, FALSE);
        return;
    }

    if (style > FRAME_STYLE_BATTLE)
        style = FRAME_STYLE_STANDARD;

    if (!sStyleGenerated[style])
        UiWindowFrame_GenerateTiles(style);

    // In the real decomp build, this would:
    // 1. Copy sStyleTiles[style] into VRAM at the window's tile base.
    // 2. Write the 3×3 tile map entries around the window rectangle.
    //
    // Pseudocode:
    //   struct Window *win = &gWindows[windowId];
    //   u16 tileBase = win->tileBase - WIN_FRAME_TILE_COUNT;
    //   LoadBgTiles(win->bg, sStyleTiles[style], sizeof(sStyleTiles[style]), tileBase);
    //   WriteFrameTilemap(win, tileBase, palSlot);
    //   CopyBgTilemapBufferToVram(win->bg);
    (void)windowId;
}

void UiWindowFrame_Clear(u8 windowId)
{
    // In real build:
    //   ClearStdWindowAndFrame(windowId, TRUE);
    (void)windowId;
}
