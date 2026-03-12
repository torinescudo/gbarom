#ifndef GUARD_ROGUELIKE_RUNTIME_H
#define GUARD_ROGUELIKE_RUNTIME_H

#include "global.h"
#include "roguelike.h"

typedef struct
{
    RogueRunState run;
    RogueHubState hub;
    RogueFloor floor;
    RogueEnemyTeam team;
    RogueBattlePlan plans[ROGUE_MAX_NODES_PER_FLOOR];
    RogueMapRef zoneChoices[ROGUE_MAX_ZONE_CHOICES];
} RogueRuntimeContext;

void RogueRuntime_BeginNewRun(RogueRuntimeContext *ctx, const RogueStartConfig *config);
void RogueRuntime_LoadFromSave(RogueRuntimeContext *ctx, const RogueSaveData *save);
void RogueRuntime_BuildCurrentFloor(RogueRuntimeContext *ctx, u8 playerAvgLevel, u8 playerPartySize);
void RogueRuntime_SelectNode(RogueRuntimeContext *ctx, u8 nodeIndex, RogueNodeResolution *outResolution, RogueGymProgress *outGymProgress);
void RogueRuntime_Save(const RogueRuntimeContext *ctx, RogueSaveData *outSave);
u8 RogueRuntime_AutoPickNode(const RogueRuntimeContext *ctx);
void RogueRuntime_PlayStep(RogueRuntimeContext *ctx, u8 playerAvgLevel, u8 playerPartySize, RogueNodeResolution *outResolution, RogueGymProgress *outGymProgress);
bool8 RogueRuntime_IsFinished(const RogueRuntimeContext *ctx);

#endif // GUARD_ROGUELIKE_RUNTIME_H
