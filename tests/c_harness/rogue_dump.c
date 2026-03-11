#include <stdio.h>
#include <stdlib.h>

#include "roguelike.h"

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
    RogueRunState st;
    RogueRunState stAfter;
    RogueFloor floor;
    RogueEnemyTeam team;
    RogueBattlePlan plan;
    RogueNodeResolution resolution;
    RogueHubState hub;
    RogueGymProgress gym;
    RogueMapRef choices[ROGUE_MAX_ZONE_CHOICES];
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

    Rogue_InitRun(&st, (u32)strtoul(argv[1], NULL, 10), totalFloors);
    st.currentFloor = floorIndex;
    Rogue_HubInit(&hub);
    Rogue_GenerateFloor(&st, &floor, floorIndex);
    Rogue_GenerateZoneChoices(&st, floorIndex, 2, choices);
    Rogue_GenerateEnemyTeam(&st, floorIndex, 15 + hub.upgradeLevel, hub.unlockedTeamSize, floorIndex == totalFloors - 1, &team);

    printf("{\n");
    printf("  \"floor\": %u,\n", floor.floorNumber);
    printf("  \"biome\": %u,\n", floor.biome);
    printf("  \"nodes\": [\n");
    for (i = 0; i < floor.nodeCount; i++)
    {
        printf("    {\"index\": %u, \"node_type\": \"%s\", \"danger\": %u, \"reward_tier\": %u, \"map_group\": %u, \"map_num\": %u}%s\n",
               i,
               NodeTypeName(floor.nodes[i].nodeType),
               floor.nodes[i].danger,
               floor.nodes[i].rewardTier,
               floor.nodes[i].map.mapGroup,
               floor.nodes[i].map.mapNum,
               (i + 1 < floor.nodeCount) ? "," : "");
    }
    printf("  ],\n");
    printf("  \"zone_choices\": [\n");
    for (i = 0; i < 2; i++)
    {
        printf("    {\"map_group\": %u, \"map_num\": %u}%s\n",
               choices[i].mapGroup,
               choices[i].mapNum,
               (i + 1 < 2) ? "," : "");
    }
    printf("  ],\n");
    printf("  \"battle_plans\": [\n");
    for (i = 0; i < floor.nodeCount; i++)
    {
        Rogue_BuildBattlePlan(&floor.nodes[i], &team, &plan);
        printf("    {\"encounter_type\": %u, \"ai_level\": %u, \"reward_tier\": %u, \"gives_heal\": %u, \"gives_shop\": %u}%s\n",
               plan.encounterType,
               plan.aiLevel,
               plan.rewardTier,
               plan.givesHeal,
               plan.givesShop,
               (i + 1 < floor.nodeCount) ? "," : "");
    }
    printf("  ],\n");

    Rogue_BuildBattlePlan(&floor.nodes[0], &team, &plan);
    Rogue_ResolveNode(&st, &floor, 0, &plan, &resolution);
    Rogue_EvaluateGymLeader(&st, &plan, &hub, &gym);
    Rogue_HubApplyResolution(&hub, &resolution);
    Rogue_HubUpgradePower(&hub);
    Rogue_HubUpgradeTeamSize(&hub);

    stAfter = st;
    Rogue_AdvanceRun(&stAfter);

    printf("  \"enemy_team\": {\"party_size\": %u, \"ai_level\": %u, \"party\": [\n",
           team.partySize,
           team.aiLevel);
    for (i = 0; i < team.partySize; i++)
    {
        printf("    {\"species\": %u, \"level\": %u}%s\n",
               team.party[i].species,
               team.party[i].level,
               (i + 1 < team.partySize) ? "," : "");
    }
    printf("  ]},\n");

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
           hub.hubCurrency,
           hub.upgradeLevel,
           hub.unlockedTeamSize,
           hub.medals);

    printf("  \"next_floor\": %u,\n", stAfter.currentFloor);
    printf("  \"run_finished\": %u\n", Rogue_RunIsFinished(&stAfter));
    printf("}\n");

    return 0;
}
