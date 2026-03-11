#include <stdio.h>
#include <stdlib.h>

#include "roguelike.h"
#include "roguelike_runtime.h"

static const char *NodeTypeName(u8 t)
{
    switch (t)
    {
    case ROGUE_NODE_WILD: return "WILD";
    case ROGUE_NODE_TRAINER: return "TRAINER";
    case ROGUE_NODE_ELITE: return "ELITE";
    case ROGUE_NODE_SHOP: return "SHOP";
    case ROGUE_NODE_REST: return "REST";
    case ROGUE_NODE_EVENT: return "EVENT";
    case ROGUE_NODE_BOSS: return "BOSS";
    default: return "WILD";
    }
}

int main(int argc, char **argv)
{
    RogueRuntimeContext ctx;
    RogueNodeResolution resolution;
    RogueGymProgress gym;
    u8 pickedNode;
    RogueSaveData saveData;
    RogueStartConfig startConfig;
    u16 floorIndex;
    u16 totalFloors;
    u16 i;

    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <seed> <floor_index> <total_floors>\n", argv[0]);
        return 2;
    }

    floorIndex = (u16)strtoul(argv[2], NULL, 10);
    totalFloors = (u16)strtoul(argv[3], NULL, 10);

    startConfig.seed = (u32)strtoul(argv[1], NULL, 10);
    startConfig.totalFloors = totalFloors;
    startConfig.startingTeamSize = 3;
    startConfig.levelBonus = 0;

    RogueRuntime_BeginNewRun(&ctx, &startConfig);
    ctx.run.currentFloor = floorIndex;
    RogueRuntime_BuildCurrentFloor(&ctx, 15, 3);
    pickedNode = RogueRuntime_AutoPickNode(&ctx);

    printf("{\n");
    printf("  \"floor\": %u,\n", ctx.floor.floorNumber);
    printf("  \"biome\": %u,\n", ctx.floor.biome);
    printf("  \"nodes\": [\n");
    for (i = 0; i < ctx.floor.nodeCount; i++)
    {
        printf("    {\"index\": %u, \"node_type\": \"%s\", \"danger\": %u, \"reward_tier\": %u, \"map_group\": %u, \"map_num\": %u}%s\n",
               i,
               NodeTypeName(ctx.floor.nodes[i].nodeType),
               ctx.floor.nodes[i].danger,
               ctx.floor.nodes[i].rewardTier,
               ctx.floor.nodes[i].map.mapGroup,
               ctx.floor.nodes[i].map.mapNum,
               (i + 1 < ctx.floor.nodeCount) ? "," : "");
    }
    printf("  ],\n");
    printf("  \"zone_choices\": [\n");
    for (i = 0; i < 2; i++)
    {
        printf("    {\"map_group\": %u, \"map_num\": %u}%s\n",
               ctx.zoneChoices[i].mapGroup,
               ctx.zoneChoices[i].mapNum,
               (i + 1 < 2) ? "," : "");
    }
    printf("  ],\n");
    printf("  \"battle_plans\": [\n");
    for (i = 0; i < ctx.floor.nodeCount; i++)
    {
        printf("    {\"encounter_type\": %u, \"ai_level\": %u, \"reward_tier\": %u, \"gives_heal\": %u, \"gives_shop\": %u}%s\n",
               ctx.plans[i].encounterType,
               ctx.plans[i].aiLevel,
               ctx.plans[i].rewardTier,
               ctx.plans[i].givesHeal,
               ctx.plans[i].givesShop,
               (i + 1 < ctx.floor.nodeCount) ? "," : "");
    }
    printf("  ],\n");

    RogueRuntime_SelectNode(&ctx, pickedNode, &resolution, &gym);

    printf("  \"enemy_team\": {\"party_size\": %u, \"ai_level\": %u, \"party\": [\n",
           ctx.team.partySize,
           ctx.team.aiLevel);
    for (i = 0; i < ctx.team.partySize; i++)
    {
        printf("    {\"species\": %u, \"level\": %u}%s\n",
               ctx.team.party[i].species,
               ctx.team.party[i].level,
               (i + 1 < ctx.team.partySize) ? "," : "");
    }
    printf("  ]},\n");

    printf("  \"selected_node\": %u,\n", pickedNode);
    printf("  \"sample_resolution\": {\"selected_node\": %u, \"reward_tier\": %u, \"gold_reward\": %u, \"healed\": %u, \"opens_shop\": %u, \"encounter_type\": %u},\n",
           resolution.selectedNode,
           resolution.rewardTier,
           resolution.goldReward,
           resolution.healed,
           resolution.opensShop,
           resolution.encounterType);
    printf("  \"gym_progress\": {\"is_gym_leader\": %u, \"medal_awarded\": %u, \"new_medal_count\": %u},\n",
           gym.isGymLeader,
           gym.medalAwarded,
           gym.newMedalCount);
    printf("  \"hub_state\": {\"hub_currency\": %u, \"upgrade_level\": %u, \"unlocked_team_size\": %u, \"medals\": %u},\n",
           ctx.hub.hubCurrency,
           ctx.hub.upgradeLevel,
           ctx.hub.unlockedTeamSize,
           ctx.hub.medals);

    RogueRuntime_Save(&ctx, &saveData);
    printf("  \"save_data\": {\"seed\": %u, \"total_floors\": %u, \"current_floor\": %u, \"hub_currency\": %u, \"hub_upgrade_level\": %u, \"hub_unlocked_team_size\": %u, \"hub_medals\": %u},\n",
           saveData.seed,
           saveData.totalFloors,
           saveData.currentFloor,
           saveData.hubCurrency,
           saveData.hubUpgradeLevel,
           saveData.hubUnlockedTeamSize,
           saveData.hubMedals);

    printf("  \"next_floor\": %u,\n", ctx.run.currentFloor);
    printf("  \"run_finished\": %u\n", Rogue_RunIsFinished(&ctx.run));
    printf("}\n");

    return 0;
}
