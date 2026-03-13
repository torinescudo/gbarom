#ifndef GUARD_UI_THEME_H
#define GUARD_UI_THEME_H

#include "global.h"
#include "constants/rgb.h"

/*
 * UI Theme system — Retro Liquid Crystal style.
 */

enum UiThemeId
{
    UI_THEME_VANILLA = 0,
    UI_THEME_LIQUID_CRYSTAL,
    UI_THEME_COUNT,
};

/* Liquid Crystal palette constants (using pokeemerald RGB2 macro) */
#define LC_TRANSPARENT      0x0000
#define LC_BG_DARK          RGB2( 4,  4,  6)
#define LC_BG_MID           RGB2( 7,  7,  9)
#define LC_BORDER_OUTER     RGB2(11, 11, 13)
#define LC_TEXT_PRIMARY     RGB2(29, 29, 28)
#define LC_ACCENT_CYAN      RGB2( 6, 28, 28)
#define LC_ACCENT_GOLD      RGB2(28, 26,  8)
#define LC_COLOR_RED        RGB2(28,  6,  6)
#define LC_COLOR_GREEN      RGB2( 6, 26,  8)
#define LC_COLOR_BLUE       RGB2( 8, 14, 28)
#define LC_TEXT_SECONDARY   RGB2(18, 18, 20)
#define LC_SHADOW           RGB2( 2,  2,  3)
#define LC_DIALOG_BG        RGB2( 5,  5,  8)
#define LC_CURSOR_BRIGHT    LC_ACCENT_CYAN
#define LC_CURSOR_DIM       RGB2( 4, 22, 22)

/* Palette slot assignments */
#define LC_PAL_SLOT_MENU_BG   13
#define LC_PAL_SLOT_DIALOG_BG 14
#define LC_PAL_SLOT_CURSOR    15

/* Window frame tile constants */
#define WIN_FRAME_TILE_COUNT  9
#define WIN_FRAME_TILE_SIZE   8

/* Textbox geometry */
#define LC_TEXTBOX_LINES      3
#define LC_TEXTBOX_HEIGHT_PX  (LC_TEXTBOX_LINES * 16)
#define LC_TEXTBOX_PAD_TOP    4
#define LC_TEXTBOX_PAD_SIDE   6
#define LC_TEXT_SPEED_DEFAULT  1

/* Cursor animation */
#define LC_CURSOR_BOUNCE_PX   2
#define LC_CURSOR_BLINK_PERIOD 16

/* Blend / fade settings */
#define LC_FADE_FRAMES        4
#define LC_BLEND_EVA          8
#define LC_BLEND_EVB          8

/* Public API */
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

#endif /* GUARD_UI_THEME_H */
