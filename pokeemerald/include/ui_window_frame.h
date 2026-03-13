#ifndef GUARD_UI_WINDOW_FRAME_H
#define GUARD_UI_WINDOW_FRAME_H

#include "global.h"

#define FRAME_STYLE_STANDARD   0
#define FRAME_STYLE_DIALOG     1
#define FRAME_STYLE_BATTLE     2

void UiWindowFrame_GenerateTiles(u8 style);
void UiWindowFrame_Draw(u8 windowId, u8 style);
void UiWindowFrame_Clear(u8 windowId);

#endif /* GUARD_UI_WINDOW_FRAME_H */
