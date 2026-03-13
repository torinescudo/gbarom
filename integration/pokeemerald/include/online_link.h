#ifndef GUARD_ONLINE_LINK_H
#define GUARD_ONLINE_LINK_H

#include "global.h"

#define ONLINE_LINK_MAX_PAYLOAD 8

enum OnlineLinkState
{
    ONLINE_LINK_STATE_IDLE = 0,
    ONLINE_LINK_STATE_CONNECTING,
    ONLINE_LINK_STATE_CONNECTED,
    ONLINE_LINK_STATE_ERROR,
};

struct OnlineLinkFrame
{
    u16 seq;
    u8 size;
    u8 payload[ONLINE_LINK_MAX_PAYLOAD];
};

void OnlineLink_Init(void);
bool8 OnlineLink_StartSession(const u8 *roomCode, u8 side);
void OnlineLink_StopSession(void);
void OnlineLink_Tick(void);
bool8 OnlineLink_IsConnected(void);

bool8 OnlineLink_SendFrame(const struct OnlineLinkFrame *frame);
bool8 OnlineLink_RecvFrame(struct OnlineLinkFrame *frameOut);
u16 OnlineLink_GetLastRemoteSeq(void);
enum OnlineLinkState OnlineLink_GetState(void);

#endif  // GUARD_ONLINE_LINK_H
