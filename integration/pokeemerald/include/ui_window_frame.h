#ifndef GUARD_UI_WINDOW_FRAME_H
#define GUARD_UI_WINDOW_FRAME_H

#include "global.h"

/*
 * Custom window frame renderer for the LC theme.
 *
 * Replaces the default 9-tile text window frame with a sleek
 * dark frame using thin borders, inner glow line, and drop shadow.
 *
 * Call UiWindowFrame_Draw() wherever pokeemerald calls
 * DrawStdWindowFrame / DrawDialogFrameWithCustomTileAndPalette.
 */

// Frame visual style flags
#define FRAME_STYLE_STANDARD   0  // Menu/info windows
#define FRAME_STYLE_DIALOG     1  // NPC textbox (slightly lighter bg)
#define FRAME_STYLE_BATTLE     2  // Battle UI panels

void UiWindowFrame_GenerateTiles(u8 style);
void UiWindowFrame_Draw(u8 windowId, u8 style);
void UiWindowFrame_Clear(u8 windowId);

#endif // GUARD_UI_WINDOW_FRAME_H
