#include "global.h"
#include "roguelike.h"
#include "roguelike_runtime.h"

void RogueRuntime_BeginNewRun(RogueRuntimeContext *ctx, const RogueStartConfig *config)
{
    Rogue_HubInit(&ctx->hub);
    Rogue_BeginRunFromHub(config, &ctx->run, &ctx->hub);
}

void RogueRuntime_LoadFromSave(RogueRuntimeContext *ctx, const RogueSaveData *save)
{
    Rogue_LoadFromData(save, &ctx->run, &ctx->hub);
}

void RogueRuntime_BuildCurrentFloor(RogueRuntimeContext *ctx, u8 playerAvgLevel, u8 playerPartySize)
{
    u8 effectiveAvgLevel = playerAvgLevel + ctx->hub.upgradeLevel;
    u8 effectivePartySize = playerPartySize;
    u8 i;

    if (effectivePartySize < ctx->hub.unlockedTeamSize)
        effectivePartySize = ctx->hub.unlockedTeamSize;

    Rogue_GenerateFloor(&ctx->run, &ctx->floor, ctx->run.currentFloor);
    Rogue_GenerateZoneChoices(&ctx->run, ctx->run.currentFloor, 2, ctx->zoneChoices);
    Rogue_GenerateEnemyTeam(
        &ctx->run,
        ctx->run.currentFloor,
        effectiveAvgLevel,
        effectivePartySize,
        ctx->run.currentFloor == ctx->run.totalFloors - 1,
        &ctx->team);

    for (i = 0; i < ctx->floor.nodeCount; i++)
        Rogue_BuildBattlePlan(&ctx->floor.nodes[i], &ctx->team, &ctx->plans[i]);
}

void RogueRuntime_SelectNode(RogueRuntimeContext *ctx, u8 nodeIndex, RogueNodeResolution *outResolution, RogueGymProgress *outGymProgress)
{
    if (nodeIndex >= ctx->floor.nodeCount)
        nodeIndex = 0;

    Rogue_ResolveNode(&ctx->run, &ctx->floor, nodeIndex, &ctx->plans[nodeIndex], outResolution);
    Rogue_EvaluateGymLeader(&ctx->run, &ctx->plans[nodeIndex], &ctx->hub, outGymProgress);
    Rogue_HubApplyResolution(&ctx->hub, outResolution);
    Rogue_HubUpgradePower(&ctx->hub);
    Rogue_HubUpgradeTeamSize(&ctx->hub);
    Rogue_AdvanceRun(&ctx->run);
}

void RogueRuntime_Save(const RogueRuntimeContext *ctx, RogueSaveData *outSave)
{
    Rogue_SaveToData(&ctx->run, &ctx->hub, outSave);
}

u8 RogueRuntime_AutoPickNode(const RogueRuntimeContext *ctx)
{
    u8 i;

    for (i = 0; i < ctx->floor.nodeCount; i++)
    {
        if (ctx->floor.nodes[i].nodeType == ROGUE_NODE_BOSS)
            return i;
    }

    for (i = 0; i < ctx->floor.nodeCount; i++)
    {
        if (ctx->floor.nodes[i].nodeType == ROGUE_NODE_ELITE)
            return i;
    }

    for (i = 0; i < ctx->floor.nodeCount; i++)
    {
        if (ctx->floor.nodes[i].nodeType == ROGUE_NODE_TRAINER)
            return i;
    }

    return 0;
}

void RogueRuntime_PlayStep(RogueRuntimeContext *ctx, u8 playerAvgLevel, u8 playerPartySize, RogueNodeResolution *outResolution, RogueGymProgress *outGymProgress)
{
    u8 pickedNode;

    RogueRuntime_BuildCurrentFloor(ctx, playerAvgLevel, playerPartySize);
    pickedNode = RogueRuntime_AutoPickNode(ctx);
    RogueRuntime_SelectNode(ctx, pickedNode, outResolution, outGymProgress);
}

bool8 RogueRuntime_IsFinished(const RogueRuntimeContext *ctx)
{
    return Rogue_RunIsFinished(&ctx->run);
}
