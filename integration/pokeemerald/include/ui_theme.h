#ifndef GUARD_UI_THEME_H
#define GUARD_UI_THEME_H

#include "global.h"

/*
 * UI Theme system for pokeemerald — Retro Liquid Crystal style.
 *
 * This module provides a switchable theme layer that replaces
 * the default Emerald UI with a modernized retro aesthetic
 * inspired by Pokémon Liquid Crystal.
 *
 * Integration: include this header and call UiTheme_Init() during
 * game initialization, before any menu or text window is drawn.
 */

// ── Theme IDs ──────────────────────────────────────────────
enum UiThemeId
{
    UI_THEME_VANILLA = 0,   // Original Emerald look
    UI_THEME_LIQUID_CRYSTAL, // Dark retro LC style
    UI_THEME_COUNT,
};

// ── 15-bit GBA color helper ────────────────────────────────
#define RGB15(r, g, b)  (((b) << 10) | ((g) << 5) | (r))

// ── Liquid Crystal palette constants ───────────────────────
// Menu background (BG pal slot 13)
#define LC_TRANSPARENT      0x0000
#define LC_BG_DARK          RGB15( 4,  4,  6)   // Deep navy
#define LC_BG_MID           RGB15( 7,  7,  9)   // Soft dark blue-grey
#define LC_BORDER_OUTER     RGB15(11, 11, 13)    // Visible border
#define LC_TEXT_PRIMARY      RGB15(29, 29, 28)    // Near-white
#define LC_ACCENT_CYAN       RGB15( 6, 28, 28)    // Signature cyan
#define LC_ACCENT_GOLD       RGB15(28, 26,  8)    // Gold highlight
#define LC_COLOR_RED         RGB15(28,  6,  6)    // Error / low HP
#define LC_COLOR_GREEN       RGB15( 6, 26,  8)    // High HP
#define LC_COLOR_BLUE        RGB15( 8, 14, 28)    // Mid HP
#define LC_TEXT_SECONDARY    RGB15(18, 18, 20)    // Dimmed text
#define LC_SHADOW            RGB15( 2,  2,  3)    // Text drop shadow

// Dialog background (BG pal slot 14)
#define LC_DIALOG_BG         RGB15( 5,  5,  8)

// Cursor sprite (OBJ pal slot 15)
#define LC_CURSOR_BRIGHT     LC_ACCENT_CYAN
#define LC_CURSOR_DIM        RGB15( 4, 22, 22)

// ── Palette slot assignments ───────────────────────────────
#define LC_PAL_SLOT_MENU_BG   13
#define LC_PAL_SLOT_DIALOG_BG 14
#define LC_PAL_SLOT_CURSOR    15

// ── Window frame tile dimensions ───────────────────────────
#define WIN_FRAME_TILE_COUNT  9    // 3×3 grid: TL T TR / L C R / BL B BR
#define WIN_FRAME_TILE_SIZE   8    // 8×8 px per tile

// ── Textbox geometry ───────────────────────────────────────
#define LC_TEXTBOX_LINES      3
#define LC_TEXTBOX_HEIGHT_PX  (LC_TEXTBOX_LINES * 16)
#define LC_TEXTBOX_PAD_TOP    4
#define LC_TEXTBOX_PAD_SIDE   6
#define LC_TEXT_SPEED_DEFAULT  1   // chars per frame

// ── Cursor animation ──────────────────────────────────────
#define LC_CURSOR_BOUNCE_PX   2
#define LC_CURSOR_BLINK_PERIOD 16  // frames between bright/dim

// ── Blend / fade settings ──────────────────────────────────
#define LC_FADE_FRAMES        4
#define LC_BLEND_EVA          8    // Alpha for highlight bar (of 16)
#define LC_BLEND_EVB          8

// ── Public API ─────────────────────────────────────────────
void UiTheme_Init(void);
void UiTheme_SetTheme(enum UiThemeId themeId);
enum UiThemeId UiTheme_GetCurrent(void);

void UiTheme_ApplyMenuPalette(void);
void UiTheme_ApplyDialogPalette(void);
void UiTheme_ApplyCursorPalette(void);
void UiTheme_ApplyAllPalettes(void);

void UiTheme_LoadWindowFrameTiles(void);
void UiTheme_DrawHighlightBar(u8 y, u8 height);
void UiTheme_SetupFadeIn(void);
void UiTheme_SetupFadeOut(void);
void UiTheme_TickFade(void);
bool8 UiTheme_IsFading(void);

void UiTheme_SetTextSpeed(u8 charsPerFrame);
u8   UiTheme_GetTextSpeed(void);
void UiTheme_TickCursorAnim(void);
s8   UiTheme_GetCursorOffsetY(void);

#endif // GUARD_UI_THEME_H
