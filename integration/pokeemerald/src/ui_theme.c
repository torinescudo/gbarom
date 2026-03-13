#include "global.h"
#include "ui_theme.h"
#include "palette.h"  // pokeemerald: LoadPalette, FillPalette, etc.
#include "gpu_regs.h" // pokeemerald: SetGpuReg for BLDCNT/BLDY

/*
 * UI Theme — implementation
 *
 * Manages palette loading, fade transitions, cursor animation
 * and theme switching between Vanilla and Liquid Crystal.
 */

// ── Static state ───────────────────────────────────────────
static enum UiThemeId sCurrentTheme;
static u8  sTextSpeed;
static bool8 sFading;
static s8  sFadeStep;       // +1 = fading in, -1 = fading out
static u8  sFadeCounter;
static u8  sFadeLevel;      // 0..16 (GBA BLDY range)
static u16 sCursorAnimTimer;
static s8  sCursorOffsetY;

// ── LC menu palette (16 entries for BG pal slot) ───────────
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

// ── Helpers ────────────────────────────────────────────────

// pokeemerald-compatible palette loader stub.
// In the real decomp, call LoadPalette(src, offset, size).
static void LoadPalette16(const u16 *src, u8 slot)
{
    // Each palette slot is 16 colors × 2 bytes = 32 bytes.
    // BG palettes start at PLTT, OBJ palettes at PLTT + 0x200.
    // Slot index: slot * 16 colors.
    // In a real build this becomes:
    //   LoadPalette(src, slot * 16, 32);
    // Stub: write to palette RAM directly (safe for GBA).
    volatile u16 *dst;
    u32 i;

    if (slot < 16)
        dst = (volatile u16 *)(0x05000000 + slot * 32); // BG palette
    else
        dst = (volatile u16 *)(0x05000200 + (slot - 16) * 32); // OBJ palette

    for (i = 0; i < 16; i++)
        dst[i] = src[i];
}

// ── Public API ─────────────────────────────────────────────

void UiTheme_Init(void)
{
    sCurrentTheme   = UI_THEME_LIQUID_CRYSTAL;
    sTextSpeed      = LC_TEXT_SPEED_DEFAULT;
    sFading         = FALSE;
    sFadeStep       = 0;
    sFadeCounter    = 0;
    sFadeLevel      = 0;
    sCursorAnimTimer = 0;
    sCursorOffsetY  = 0;

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

// ── Palette application ────────────────────────────────────

void UiTheme_ApplyMenuPalette(void)
{
    if (sCurrentTheme == UI_THEME_LIQUID_CRYSTAL)
        LoadPalette16(sLcMenuPalette, LC_PAL_SLOT_MENU_BG);
    // else: do nothing; vanilla palettes are already loaded by decomp.
}

void UiTheme_ApplyDialogPalette(void)
{
    if (sCurrentTheme == UI_THEME_LIQUID_CRYSTAL)
        LoadPalette16(sLcDialogPalette, LC_PAL_SLOT_DIALOG_BG);
}

void UiTheme_ApplyCursorPalette(void)
{
    if (sCurrentTheme == UI_THEME_LIQUID_CRYSTAL)
        LoadPalette16(sLcCursorPalette, 16 + LC_PAL_SLOT_CURSOR); // OBJ slot
}

void UiTheme_ApplyAllPalettes(void)
{
    UiTheme_ApplyMenuPalette();
    UiTheme_ApplyDialogPalette();
    UiTheme_ApplyCursorPalette();
}

// ── Window frame tiles ─────────────────────────────────────

/*
 * Generate 9 tiles (3×3) procedurally for the LC frame style.
 * Each tile is 8×8 @ 4bpp = 32 bytes.
 *
 * Palette index mapping:
 *   0 = transparent
 *   1 = bg_dark (fill)
 *   3 = border_outer (1px edge)
 *   5 = accent_cyan (inner glow line)
 *   11 = shadow (1px offset)
 */

// Pixel helper: set a 4bpp pixel in a tile buffer.
static void SetTilePixel(u8 *tile, u8 x, u8 y, u8 palIdx)
{
    // 4bpp: each byte holds 2 pixels (low nibble = left pixel).
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

// Generate a single frame tile based on position (0..8 in row-major 3×3).
static void GenerateFrameTile(u8 *tile, u8 pos)
{
    u8 x, y;
    bool8 isTop    = (pos < 3);
    bool8 isBottom = (pos >= 6);
    bool8 isLeft   = (pos % 3 == 0);
    bool8 isRight  = (pos % 3 == 2);

    // Base fill: dark background.
    FillTile(tile, 1);

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            // Outer border (1px).
            if ((isTop && y == 0) || (isBottom && y == 7)
                || (isLeft && x == 0) || (isRight && x == 7))
            {
                SetTilePixel(tile, x, y, 3); // border_outer
            }

            // Inner glow line (1px inside border).
            if ((isTop && y == 1 && x > 0 && (!isRight || x < 7))
                || (isBottom && y == 6 && x > 0 && (!isRight || x < 7))
                || (isLeft && x == 1 && y > 0 && (!isBottom || y < 7))
                || (isRight && x == 6 && y > 0 && (!isBottom || y < 7)))
            {
                SetTilePixel(tile, x, y, 5); // accent_cyan
            }

            // Drop shadow (bottom and right edges, outermost pixel).
            if ((isBottom && y == 7 && x > 0) || (isRight && x == 7 && y > 0))
            {
                SetTilePixel(tile, x, y, 11); // shadow
            }

            // Corner radius: clip the very corner 2×2 pixels to transparent.
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

// Tile buffer: 9 tiles × 32 bytes each = 288 bytes total.
static u8 sFrameTileBuffer[WIN_FRAME_TILE_COUNT * 32];

void UiTheme_LoadWindowFrameTiles(void)
{
    u8 i;

    if (sCurrentTheme != UI_THEME_LIQUID_CRYSTAL)
        return;

    for (i = 0; i < WIN_FRAME_TILE_COUNT; i++)
        GenerateFrameTile(&sFrameTileBuffer[i * 32], i);

    // In a real build, copy sFrameTileBuffer into VRAM at the
    // text window tile base using LoadBgTiles / CopyToBgTilemapBuffer.
    // Stub: direct VRAM write.
    // CopyToVram(BG_CHAR_ADDR(2) + tileBase * 32, sFrameTileBuffer, sizeof(sFrameTileBuffer));
}

// ── Highlight bar (alpha-blended selection) ────────────────

void UiTheme_DrawHighlightBar(u8 y, u8 height)
{
    if (sCurrentTheme != UI_THEME_LIQUID_CRYSTAL)
        return;

    // Configure GBA hardware alpha blending.
    // BLD target 1 = BG1 (menu), target 2 = BG0 (backdrop).
    // EVA / EVB control translucency.
    //
    // In real build:
    //   SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0);
    //   SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(LC_BLEND_EVA, LC_BLEND_EVB));
    //
    // Then draw a filled rectangle at (0, y) → (240, y+height) using
    // the accent cyan color on BG1.
    (void)y;
    (void)height;
}

// ── Fade transitions ───────────────────────────────────────

void UiTheme_SetupFadeIn(void)
{
    sFading      = TRUE;
    sFadeStep    = -1;   // brightness decreasing → screen appearing
    sFadeLevel   = 16;
    sFadeCounter = 0;

    // SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_ALL | BLDCNT_EFFECT_LIGHTEN);
    // SetGpuReg(REG_OFFSET_BLDY, 16);
}

void UiTheme_SetupFadeOut(void)
{
    sFading      = TRUE;
    sFadeStep    = 1;    // brightness increasing → screen disappearing
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

    // SetGpuReg(REG_OFFSET_BLDY, sFadeLevel);

    if ((sFadeStep > 0 && sFadeLevel >= 16) || (sFadeStep < 0 && sFadeLevel == 0))
    {
        sFading = FALSE;
        // SetGpuReg(REG_OFFSET_BLDCNT, 0);
        // SetGpuReg(REG_OFFSET_BLDY, 0);
    }
}

bool8 UiTheme_IsFading(void)
{
    return sFading;
}

// ── Text speed ─────────────────────────────────────────────

void UiTheme_SetTextSpeed(u8 charsPerFrame)
{
    sTextSpeed = (charsPerFrame > 0) ? charsPerFrame : 1;
}

u8 UiTheme_GetTextSpeed(void)
{
    return sTextSpeed;
}

// ── Cursor animation (bouncing triangle) ───────────────────

void UiTheme_TickCursorAnim(void)
{
    sCursorAnimTimer++;

    // Simple sine-ish bounce: 0 → +2 → 0 → +2 …
    // Period = LC_CURSOR_BLINK_PERIOD frames.
    u16 phase = sCursorAnimTimer % (LC_CURSOR_BLINK_PERIOD * 2);

    if (phase < LC_CURSOR_BLINK_PERIOD)
        sCursorOffsetY = (s8)((phase * LC_CURSOR_BOUNCE_PX) / LC_CURSOR_BLINK_PERIOD);
    else
        sCursorOffsetY = (s8)(LC_CURSOR_BOUNCE_PX - ((phase - LC_CURSOR_BLINK_PERIOD) * LC_CURSOR_BOUNCE_PX) / LC_CURSOR_BLINK_PERIOD);
}

s8 UiTheme_GetCursorOffsetY(void)
{
    return sCursorOffsetY;
}
