#include "global.h"
#include "online_link.h"

#define ONLINE_LINK_QUEUE_SIZE 64

struct OnlineLinkQueue
{
    struct OnlineLinkFrame items[ONLINE_LINK_QUEUE_SIZE];
    u16 head;
    u16 tail;
    u16 count;
};

static enum OnlineLinkState sOnlineState;
static u8 sOnlineSide;
static u16 sLastRemoteSeq;
static struct OnlineLinkQueue sTxQueue;
static struct OnlineLinkQueue sRxQueue;

static void QueueReset(struct OnlineLinkQueue *q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

static bool8 QueuePush(struct OnlineLinkQueue *q, const struct OnlineLinkFrame *frame)
{
    if (q->count >= ONLINE_LINK_QUEUE_SIZE)
        return FALSE;

    q->items[q->tail] = *frame;
    q->tail = (q->tail + 1) % ONLINE_LINK_QUEUE_SIZE;
    q->count++;
    return TRUE;
}

static bool8 QueuePop(struct OnlineLinkQueue *q, struct OnlineLinkFrame *frameOut)
{
    if (q->count == 0)
        return FALSE;

    *frameOut = q->items[q->head];
    q->head = (q->head + 1) % ONLINE_LINK_QUEUE_SIZE;
    q->count--;
    return TRUE;
}

void OnlineLink_Init(void)
{
    sOnlineState = ONLINE_LINK_STATE_IDLE;
    sOnlineSide = 0;
    sLastRemoteSeq = 0;
    QueueReset(&sTxQueue);
    QueueReset(&sRxQueue);
}

bool8 OnlineLink_StartSession(const u8 *roomCode, u8 side)
{
    if (roomCode == NULL)
        return FALSE;

    sOnlineSide = side;
    sOnlineState = ONLINE_LINK_STATE_CONNECTING;

    // TODO: abrir socket/bridge real y completar handshake de protocolo.
    sOnlineState = ONLINE_LINK_STATE_CONNECTED;
    return TRUE;
}

void OnlineLink_StopSession(void)
{
    // TODO: cerrar transporte real y limpiar recursos.
    sOnlineState = ONLINE_LINK_STATE_IDLE;
    QueueReset(&sTxQueue);
    QueueReset(&sRxQueue);
}

void OnlineLink_Tick(void)
{
    struct OnlineLinkFrame frame;

    if (sOnlineState != ONLINE_LINK_STATE_CONNECTED)
        return;

    // Stub temporal: loopback de TX->RX para pruebas de integracion local.
    if (QueuePop(&sTxQueue, &frame))
    {
        sLastRemoteSeq = frame.seq;
        QueuePush(&sRxQueue, &frame);
    }
}

bool8 OnlineLink_IsConnected(void)
{
    return sOnlineState == ONLINE_LINK_STATE_CONNECTED;
}

bool8 OnlineLink_SendFrame(const struct OnlineLinkFrame *frame)
{
    if (sOnlineState != ONLINE_LINK_STATE_CONNECTED || frame == NULL)
        return FALSE;

    if (frame->size > ONLINE_LINK_MAX_PAYLOAD)
        return FALSE;

    return QueuePush(&sTxQueue, frame);
}

bool8 OnlineLink_RecvFrame(struct OnlineLinkFrame *frameOut)
{
    if (sOnlineState != ONLINE_LINK_STATE_CONNECTED || frameOut == NULL)
        return FALSE;

    return QueuePop(&sRxQueue, frameOut);
}

u16 OnlineLink_GetLastRemoteSeq(void)
{
    return sLastRemoteSeq;
}

enum OnlineLinkState OnlineLink_GetState(void)
{
    return sOnlineState;
}
