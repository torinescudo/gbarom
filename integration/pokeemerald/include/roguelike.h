#ifndef GUARD_ROGUELIKE_H
#define GUARD_ROGUELIKE_H

#include "global.h"

#define ROGUE_MAX_NODES_PER_FLOOR 5
#define ROGUE_MIN_NODES_PER_FLOOR 3
#define ROGUE_MAX_ZONE_CHOICES 3
#define ROGUE_MAX_ENEMY_TEAM_SIZE 6
#define ROGUE_MAX_MEDALS 8

enum RogueBiome
{
    ROGUE_BIOME_FOREST = 0,
    ROGUE_BIOME_MOUNTAIN,
    ROGUE_BIOME_RUINS,
};

enum RogueNodeType
{
    ROGUE_NODE_WILD = 0,
    ROGUE_NODE_TRAINER,
    ROGUE_NODE_ELITE,
    ROGUE_NODE_SHOP,
    ROGUE_NODE_REST,
    ROGUE_NODE_EVENT,
    ROGUE_NODE_BOSS,
};

enum RogueAiLevel
{
    ROGUE_AI_BASIC = 0,
    ROGUE_AI_STANDARD,
    ROGUE_AI_ADVANCED,
    ROGUE_AI_BOSS,
};

enum RogueEncounterType
{
    ROGUE_ENCOUNTER_WILD = 0,
    ROGUE_ENCOUNTER_TRAINER,
    ROGUE_ENCOUNTER_ELITE,
    ROGUE_ENCOUNTER_BOSS,
    ROGUE_ENCOUNTER_SUPPORT,
    ROGUE_ENCOUNTER_EVENT,
};

typedef struct
{
    u8 mapGroup;
    u8 mapNum;
} RogueMapRef;

typedef struct
{
    u8 nodeType;
    u8 danger;
    u8 rewardTier;
    RogueMapRef map;
} RogueNode;

typedef struct
{
    u16 species;
    u8 level;
} RogueEnemyMon;

typedef struct
{
    u8 partySize;
    u8 aiLevel;
    RogueEnemyMon party[ROGUE_MAX_ENEMY_TEAM_SIZE];
} RogueEnemyTeam;

typedef struct
{
    u8 encounterType;
    u8 aiLevel;
    u8 rewardTier;
    u8 givesHeal;
    u8 givesShop;
} RogueBattlePlan;

typedef struct
{
    u8 selectedNode;
    u8 rewardTier;
    u16 goldReward;
    u8 healed;
    u8 opensShop;
    u8 encounterType;
} RogueNodeResolution;

typedef struct
{
    u8 floorNumber;
    u8 biome;
    u8 nodeCount;
    RogueNode nodes[ROGUE_MAX_NODES_PER_FLOOR];
} RogueFloor;

typedef struct
{
    u16 hubCurrency;
    u8 upgradeLevel;
    u8 unlockedTeamSize;
    u8 medals;
} RogueHubState;

typedef struct
{
    u8 isGymLeader;
    u8 medalAwarded;
    u8 newMedalCount;
} RogueGymProgress;

typedef struct
{
    u32 seed;
    u16 totalFloors;
    u16 currentFloor;
} RogueRunState;

typedef struct
{
    u32 seed;
    u16 totalFloors;
    u16 currentFloor;
    u16 hubCurrency;
    u8 hubUpgradeLevel;
    u8 hubUnlockedTeamSize;
    u8 hubMedals;
} RogueSaveData;

typedef struct
{
    u32 seed;
    u16 totalFloors;
    u8 startingTeamSize;
    u8 levelBonus;
} RogueStartConfig;

void Rogue_InitRun(RogueRunState *state, u32 seed, u16 totalFloors);
void Rogue_BeginRunFromHub(const RogueStartConfig *config, RogueRunState *state, RogueHubState *hub);
void Rogue_GenerateFloor(const RogueRunState *state, RogueFloor *outFloor, u16 floorIndex);
void Rogue_GenerateZoneChoices(const RogueRunState *state, u16 floorIndex, u8 choiceCount, RogueMapRef *outChoices);
void Rogue_GenerateEnemyTeam(const RogueRunState *state, u16 floorIndex, u8 playerAvgLevel, u8 playerPartySize, bool8 isBoss, RogueEnemyTeam *outTeam);
void Rogue_BuildBattlePlan(const RogueNode *node, const RogueEnemyTeam *team, RogueBattlePlan *outPlan);
void Rogue_ResolveNode(const RogueRunState *state, const RogueFloor *floor, u8 nodeIndex, const RogueBattlePlan *plan, RogueNodeResolution *outResolution);
void Rogue_AdvanceRun(RogueRunState *state);
bool8 Rogue_RunIsFinished(const RogueRunState *state);
void Rogue_HubInit(RogueHubState *hub);
void Rogue_HubApplyResolution(RogueHubState *hub, const RogueNodeResolution *resolution);
void Rogue_HubUpgradeTeamSize(RogueHubState *hub);
void Rogue_HubUpgradePower(RogueHubState *hub);
void Rogue_EvaluateGymLeader(const RogueRunState *state, const RogueBattlePlan *plan, RogueHubState *hub, RogueGymProgress *outProgress);
void Rogue_SaveToData(const RogueRunState *state, const RogueHubState *hub, RogueSaveData *outData);
void Rogue_LoadFromData(const RogueSaveData *data, RogueRunState *outState, RogueHubState *outHub);

#endif // GUARD_ROGUELIKE_H
