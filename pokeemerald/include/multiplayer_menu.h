#ifndef GUARD_MULTIPLAYER_MENU_H
#define GUARD_MULTIPLAYER_MENU_H

#include "global.h"

/*
 * In-game multiplayer interaction menu.
 *
 * When the local player presses A facing the remote player's avatar,
 * this menu appears with options:
 *   - Battle
 *   - Trade
 *   - Player Card
 *   - Cancel
 *
 * Also handles incoming interaction requests (popup prompt).
 */

void MultiplayerMenu_Open(void);
void MultiplayerMenu_HandleIncomingRequest(void);
bool8 MultiplayerMenu_IsOpen(void);

// Called from field_control_avatar.c when player interacts with remote obj
bool8 MultiplayerMenu_CheckAndOpenOnInteraction(u8 objectEventId);

// Minimal script: releaseall + end. Used as return value when
// interaction is consumed by the multiplayer menu.
extern const u8 gMultiplayerInteractScript[];

#endif // GUARD_MULTIPLAYER_MENU_H
