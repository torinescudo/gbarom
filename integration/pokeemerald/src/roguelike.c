#include "global.h"
#include "roguelike.h"

struct RogueNodeWeights
{
    u8 wild;
    u8 trainer;
    u8 elite;
    u8 shop;
    u8 rest;
    u8 event;
};

static const RogueMapRef sForestMaps[] = {
    {0, 0}, {0, 1}, {0, 2}, {0, 3},
};

static const RogueMapRef sMountainMaps[] = {
    {0, 10}, {0, 11}, {0, 12}, {0, 13},
};

static const RogueMapRef sRuinsMaps[] = {
    {0, 20}, {0, 21}, {0, 22}, {0, 23},
};

static const u16 sForestSpecies[] = {16, 19, 25, 261, 263, 285};
static const u16 sMountainSpecies[] = {66, 74, 95, 299, 304, 307};
static const u16 sRuinsSpecies[] = {92, 104, 200, 343, 344, 355};

static u32 Rogue_LcgNext(u32 *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static u32 Rogue_RandRange(u32 *state, u32 n)
{
    if (n == 0)
        return 0;

    return Rogue_LcgNext(state) % n;
}

static u8 Rogue_ClampU8(u8 value, u8 min, u8 max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

static u8 Rogue_BiomeForFloor(u16 floorIndex)
{
    if (floorIndex < 8)
        return ROGUE_BIOME_FOREST;
    if (floorIndex < 16)
        return ROGUE_BIOME_MOUNTAIN;
    return ROGUE_BIOME_RUINS;
}

static const RogueMapRef *Rogue_GetMapPool(u8 biome, u8 *count)
{
    if (biome == ROGUE_BIOME_FOREST)
    {
        *count = ARRAY_COUNT(sForestMaps);
        return sForestMaps;
    }
    if (biome == ROGUE_BIOME_MOUNTAIN)
    {
        *count = ARRAY_COUNT(sMountainMaps);
        return sMountainMaps;
    }

    *count = ARRAY_COUNT(sRuinsMaps);
    return sRuinsMaps;
}

static const u16 *Rogue_GetSpeciesPool(u8 biome, u8 *count)
{
    if (biome == ROGUE_BIOME_FOREST)
    {
        *count = ARRAY_COUNT(sForestSpecies);
        return sForestSpecies;
    }
    if (biome == ROGUE_BIOME_MOUNTAIN)
    {
        *count = ARRAY_COUNT(sMountainSpecies);
        return sMountainSpecies;
    }

    *count = ARRAY_COUNT(sRuinsSpecies);
    return sRuinsSpecies;
}

static u8 Rogue_WeightedPick(u32 *rngState, const struct RogueNodeWeights *weights)
{
    u16 total = weights->wild + weights->trainer + weights->elite + weights->shop + weights->rest + weights->event;
    u16 roll = Rogue_RandRange(rngState, total);

    if (roll < weights->wild)
        return ROGUE_NODE_WILD;
    roll -= weights->wild;

    if (roll < weights->trainer)
        return ROGUE_NODE_TRAINER;
    roll -= weights->trainer;

    if (roll < weights->elite)
        return ROGUE_NODE_ELITE;
    roll -= weights->elite;

    if (roll < weights->shop)
        return ROGUE_NODE_SHOP;
    roll -= weights->shop;

    if (roll < weights->rest)
        return ROGUE_NODE_REST;

    return ROGUE_NODE_EVENT;
}

void Rogue_InitRun(RogueRunState *state, u32 seed, u16 totalFloors)
{
    state->seed = seed;
    state->totalFloors = totalFloors;
    state->currentFloor = 0;
}

void Rogue_GenerateFloor(const RogueRunState *state, RogueFloor *outFloor, u16 floorIndex)
{
    struct RogueNodeWeights weights = {
        .wild = 38,
        .trainer = 30,
        .elite = 8,
        .shop = 8,
        .rest = 8,
        .event = 8,
    };
    u32 rngState = state->seed;
    u16 i;
    u8 supportInserted = FALSE;
    u8 mapPoolCount = 0;
    const RogueMapRef *mapPool;

    for (i = 0; i <= floorIndex; i++)
        Rogue_LcgNext(&rngState);

    if (floorIndex == 0)
        weights.elite = 0;

    if (floorIndex > 0 && (floorIndex % 4) == 0)
    {
        weights.shop += 6;
        weights.rest += 6;
    }

    outFloor->floorNumber = floorIndex + 1;
    outFloor->biome = Rogue_BiomeForFloor(floorIndex);
    outFloor->nodeCount = ROGUE_MIN_NODES_PER_FLOOR + Rogue_RandRange(&rngState, 3);
    mapPool = Rogue_GetMapPool(outFloor->biome, &mapPoolCount);

    for (i = 0; i < outFloor->nodeCount; i++)
    {
        u8 nodeType = Rogue_WeightedPick(&rngState, &weights);
        u8 danger = 1 + floorIndex;
        u8 rewardTier = 1 + (floorIndex / 3);
        u8 mapIdx;

        if (floorIndex > 0 && (floorIndex % 4) == 0 && i == outFloor->nodeCount - 1 && !supportInserted)
        {
            nodeType = Rogue_RandRange(&rngState, 2) == 0 ? ROGUE_NODE_SHOP : ROGUE_NODE_REST;
            supportInserted = TRUE;
        }

        if (state->totalFloors > 0 && floorIndex == state->totalFloors - 1 && i == outFloor->nodeCount - 1)
            nodeType = ROGUE_NODE_BOSS;

        if (nodeType == ROGUE_NODE_ELITE || nodeType == ROGUE_NODE_BOSS)
        {
            danger += 1;
            rewardTier += 1;
        }

        if (rewardTier > 5)
            rewardTier = 5;

        mapIdx = Rogue_RandRange(&rngState, mapPoolCount);

        outFloor->nodes[i].nodeType = nodeType;
        outFloor->nodes[i].danger = danger;
        outFloor->nodes[i].rewardTier = rewardTier;
        outFloor->nodes[i].map = mapPool[mapIdx];
    }

    for (; i < ROGUE_MAX_NODES_PER_FLOOR; i++)
    {
        outFloor->nodes[i].nodeType = ROGUE_NODE_WILD;
        outFloor->nodes[i].danger = 0;
        outFloor->nodes[i].rewardTier = 0;
        outFloor->nodes[i].map.mapGroup = 0;
        outFloor->nodes[i].map.mapNum = 0;
    }
}

void Rogue_GenerateZoneChoices(const RogueRunState *state, u16 floorIndex, u8 choiceCount, RogueMapRef *outChoices)
{
    u32 rngState = state->seed ^ 0xA5A5A5A5u;
    u8 nextBiome = Rogue_BiomeForFloor(floorIndex + 1);
    u8 mapPoolCount = 0;
    const RogueMapRef *mapPool = Rogue_GetMapPool(nextBiome, &mapPoolCount);
    u8 used[4] = {FALSE, FALSE, FALSE, FALSE};
    u16 i;

    if (choiceCount > ROGUE_MAX_ZONE_CHOICES)
        choiceCount = ROGUE_MAX_ZONE_CHOICES;

    for (i = 0; i <= floorIndex; i++)
        Rogue_LcgNext(&rngState);

    for (i = 0; i < choiceCount; i++)
    {
        u8 attempts = 0;
        u8 idx = Rogue_RandRange(&rngState, mapPoolCount);
        while (used[idx] && attempts < 8)
        {
            idx = Rogue_RandRange(&rngState, mapPoolCount);
            attempts++;
        }

        used[idx] = TRUE;
        outChoices[i] = mapPool[idx];
    }
}

void Rogue_GenerateEnemyTeam(const RogueRunState *state, u16 floorIndex, u8 playerAvgLevel, u8 playerPartySize, bool8 isBoss, RogueEnemyTeam *outTeam)
{
    u32 rngState = state->seed ^ 0x3C3C3C3Cu;
    u8 biome = Rogue_BiomeForFloor(floorIndex);
    u8 speciesCount = 0;
    const u16 *speciesPool = Rogue_GetSpeciesPool(biome, &speciesCount);
    u8 baseParty = 2 + (floorIndex / 3);
    u8 i;

    for (i = 0; i <= floorIndex; i++)
        Rogue_LcgNext(&rngState);

    if (isBoss)
        baseParty += 2;

    if (playerPartySize > baseParty)
        baseParty = playerPartySize;

    outTeam->partySize = Rogue_ClampU8(baseParty, 1, ROGUE_MAX_ENEMY_TEAM_SIZE);

    if (isBoss)
        outTeam->aiLevel = ROGUE_AI_BOSS;
    else if (floorIndex >= 12)
        outTeam->aiLevel = ROGUE_AI_ADVANCED;
    else if (floorIndex >= 4)
        outTeam->aiLevel = ROGUE_AI_STANDARD;
    else
        outTeam->aiLevel = ROGUE_AI_BASIC;

    for (i = 0; i < outTeam->partySize; i++)
    {
        u8 variance = Rogue_RandRange(&rngState, 7);
        u8 level;

        if (playerAvgLevel <= 3)
            level = 3;
        else
            level = playerAvgLevel - 3;

        level += variance;
        level += floorIndex / 2;

        if (isBoss)
            level += 2;

        level = Rogue_ClampU8(level, 2, 100);
        outTeam->party[i].species = speciesPool[Rogue_RandRange(&rngState, speciesCount)];
        outTeam->party[i].level = level;
    }

    for (; i < ROGUE_MAX_ENEMY_TEAM_SIZE; i++)
    {
        outTeam->party[i].species = 0;
        outTeam->party[i].level = 0;
    }
}


void Rogue_BuildBattlePlan(const RogueNode *node, const RogueEnemyTeam *team, RogueBattlePlan *outPlan)
{
    outPlan->aiLevel = team->aiLevel;
    outPlan->rewardTier = node->rewardTier;
    outPlan->givesHeal = FALSE;
    outPlan->givesShop = FALSE;

    switch (node->nodeType)
    {
    case ROGUE_NODE_WILD:
        outPlan->encounterType = ROGUE_ENCOUNTER_WILD;
        break;
    case ROGUE_NODE_TRAINER:
        outPlan->encounterType = ROGUE_ENCOUNTER_TRAINER;
        break;
    case ROGUE_NODE_ELITE:
        outPlan->encounterType = ROGUE_ENCOUNTER_ELITE;
        outPlan->aiLevel = ROGUE_AI_ADVANCED;
        break;
    case ROGUE_NODE_BOSS:
        outPlan->encounterType = ROGUE_ENCOUNTER_BOSS;
        outPlan->aiLevel = ROGUE_AI_BOSS;
        outPlan->rewardTier = 5;
        break;
    case ROGUE_NODE_SHOP:
        outPlan->encounterType = ROGUE_ENCOUNTER_SUPPORT;
        outPlan->givesShop = TRUE;
        break;
    case ROGUE_NODE_REST:
        outPlan->encounterType = ROGUE_ENCOUNTER_SUPPORT;
        outPlan->givesHeal = TRUE;
        break;
    case ROGUE_NODE_EVENT:
    default:
        outPlan->encounterType = ROGUE_ENCOUNTER_EVENT;
        break;
    }
}


void Rogue_ResolveNode(const RogueRunState *state, const RogueFloor *floor, u8 nodeIndex, const RogueBattlePlan *plan, RogueNodeResolution *outResolution)
{
    u32 rngState = state->seed ^ 0x5F3759DFu;
    u16 i;
    u16 floorIndex = state->currentFloor;
    u16 baseGold;

    if (nodeIndex >= floor->nodeCount)
        nodeIndex = 0;

    for (i = 0; i <= floorIndex + nodeIndex; i++)
        Rogue_LcgNext(&rngState);

    baseGold = (u16)(80 + (plan->rewardTier * 40) + (floorIndex * 12));

    outResolution->selectedNode = nodeIndex;
    outResolution->rewardTier = plan->rewardTier;
    outResolution->goldReward = baseGold + (u16)Rogue_RandRange(&rngState, 51);
    outResolution->healed = plan->givesHeal;
    outResolution->opensShop = plan->givesShop;
    outResolution->encounterType = plan->encounterType;
}

void Rogue_AdvanceRun(RogueRunState *state)
{
    if (state->currentFloor < state->totalFloors)
        state->currentFloor++;
}

bool8 Rogue_RunIsFinished(const RogueRunState *state)
{
    return state->currentFloor >= state->totalFloors;
}


void Rogue_HubInit(RogueHubState *hub)
{
    hub->hubCurrency = 0;
    hub->upgradeLevel = 0;
    hub->unlockedTeamSize = 3;
    hub->medals = 0;
}

void Rogue_HubApplyResolution(RogueHubState *hub, const RogueNodeResolution *resolution)
{
    hub->hubCurrency += resolution->goldReward;

    if (resolution->healed)
        hub->hubCurrency += 25;

    if (resolution->opensShop)
        hub->hubCurrency += 15;
}

void Rogue_HubUpgradeTeamSize(RogueHubState *hub)
{
    u16 cost = 180 + (hub->unlockedTeamSize * 40);
    if (hub->unlockedTeamSize >= 6)
        return;

    if (hub->hubCurrency >= cost)
    {
        hub->hubCurrency -= cost;
        hub->unlockedTeamSize++;
    }
}

void Rogue_HubUpgradePower(RogueHubState *hub)
{
    u16 cost = 160 + (hub->upgradeLevel * 60);
    if (hub->upgradeLevel >= 10)
        return;

    if (hub->hubCurrency >= cost)
    {
        hub->hubCurrency -= cost;
        hub->upgradeLevel++;
    }
}

void Rogue_EvaluateGymLeader(const RogueRunState *state, const RogueBattlePlan *plan, RogueHubState *hub, RogueGymProgress *outProgress)
{
    outProgress->isGymLeader = FALSE;
    outProgress->medalAwarded = FALSE;
    outProgress->newMedalCount = hub->medals;

    if (hub->medals >= ROGUE_MAX_MEDALS)
        return;

    if (state->currentFloor > 0
        && (state->currentFloor % 4) == 0
        && (plan->encounterType == ROGUE_ENCOUNTER_TRAINER
            || plan->encounterType == ROGUE_ENCOUNTER_ELITE
            || plan->encounterType == ROGUE_ENCOUNTER_BOSS))
    {
        outProgress->isGymLeader = TRUE;
        outProgress->medalAwarded = TRUE;
        hub->medals++;
        outProgress->newMedalCount = hub->medals;
    }
}
