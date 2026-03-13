#ifndef GUARD_ONLINE_MENU_H
#define GUARD_ONLINE_MENU_H

#include "global.h"

/*
 * In-game Online menu — accessible from the Start menu.
 *
 * Provides:
 *   - Connect / Disconnect
 *   - Room code entry
 *   - Connection status display
 *   - Test Mode (spawn fake remote player for testing)
 */

void OnlineMenu_Open(void);
void CB2_OnlineMenu(void);

/* Test mode: spawns a mirror of the local player as remote */
void OnlineMenu_ToggleTestMode(void);
bool8 OnlineMenu_IsTestMode(void);

#endif /* GUARD_ONLINE_MENU_H */
