#ifndef GUARD_UI_TEXTBOX_H
#define GUARD_UI_TEXTBOX_H

#include "global.h"

struct UiTextboxConfig
{
    u8 windowId;
    u8 lines;
    u8 padTop;
    u8 padSide;
    u8 textSpeed;
    bool8 showShadow;
};

void UiTextbox_Init(struct UiTextboxConfig *cfg);
void UiTextbox_Open(u8 windowId);
void UiTextbox_Close(u8 windowId);
void UiTextbox_Print(u8 windowId, const u8 *text);
void UiTextbox_TickAdvanceIndicator(u8 windowId);
bool8 UiTextbox_IsAnimating(u8 windowId);
void UiTextbox_SetSpeed(u8 windowId, u8 charsPerFrame);

#endif /* GUARD_UI_TEXTBOX_H */
