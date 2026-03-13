#include "global.h"
#include "ui_window_frame.h"
#include "ui_theme.h"

/*
 * Window frame drawing — LC theme (C89 compatible).
 */

#define TILE_BYTES_4BPP 32

static u8 sStyleTiles[3][WIN_FRAME_TILE_COUNT * TILE_BYTES_4BPP];
static bool8 sStyleGenerated[3];

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

static u8 GetBgIndex(u8 style)
{
    return (style == FRAME_STYLE_DIALOG) ? 2 : 1;
}

static u8 GetGlowIndex(u8 style)
{
    return (style == FRAME_STYLE_BATTLE) ? 6 : 5;
}

static void GenerateStyledTile(u8 *tile, u8 pos, u8 style)
{
    u8 x, y;
    u8 bgIdx     = GetBgIndex(style);
    u8 glowIdx   = GetGlowIndex(style);

    bool8 isTop    = (pos < 3);
    bool8 isBottom = (pos >= 6);
    bool8 isLeft   = (pos % 3 == 0);
    bool8 isRight  = (pos % 3 == 2);

    WF_FillTile(tile, bgIdx);

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            if ((isTop && y == 0) || (isBottom && y == 7)
                || (isLeft && x == 0) || (isRight && x == 7))
            {
                WF_SetPixel(tile, x, y, 3); /* border_outer */
            }

            if ((isTop && y == 1 && !(isLeft && x == 0) && !(isRight && x == 7))
                || (isBottom && y == 6 && !(isLeft && x == 0) && !(isRight && x == 7))
                || (isLeft && x == 1 && !(isTop && y == 0) && !(isBottom && y == 7))
                || (isRight && x == 6 && !(isTop && y == 0) && !(isBottom && y == 7)))
            {
                WF_SetPixel(tile, x, y, glowIdx);
            }

            if ((isBottom && y == 7 && x > 0) || (isRight && x == 7 && y > 0))
            {
                WF_SetPixel(tile, x, y, 11); /* shadow */
            }

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
        return;

    if (style > FRAME_STYLE_BATTLE)
        style = FRAME_STYLE_STANDARD;

    if (!sStyleGenerated[style])
        UiWindowFrame_GenerateTiles(style);

    (void)windowId;
}

void UiWindowFrame_Clear(u8 windowId)
{
    (void)windowId;
}
