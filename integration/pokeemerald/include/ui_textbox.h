#ifndef GUARD_UI_TEXTBOX_H
#define GUARD_UI_TEXTBOX_H

#include "global.h"

/*
 * Modernised textbox for the LC theme.
 *
 * Features over vanilla:
 * - 3-line height with more breathing room.
 * - Animated advance indicator (bouncing triangle).
 * - Configurable scroll speed.
 * - Drop shadow on each glyph.
 */

struct UiTextboxConfig
{
    u8 windowId;
    u8 lines;        // visible line count (default 3)
    u8 padTop;       // px
    u8 padSide;      // px
    u8 textSpeed;    // chars per frame
    bool8 showShadow;
};

void UiTextbox_Init(struct UiTextboxConfig *cfg);
void UiTextbox_Open(u8 windowId);
void UiTextbox_Close(u8 windowId);
void UiTextbox_Print(u8 windowId, const u8 *text);
void UiTextbox_TickAdvanceIndicator(u8 windowId);
bool8 UiTextbox_IsAnimating(u8 windowId);
void UiTextbox_SetSpeed(u8 windowId, u8 charsPerFrame);

#endif // GUARD_UI_TEXTBOX_H
