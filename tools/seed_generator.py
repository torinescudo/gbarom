#!/usr/bin/env python3
"""Generador procedural determinista para runs roguelike estilo Pokémon GBA."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass, asdict
from typing import Dict, List


NODE_WEIGHTS_BASE: Dict[str, int] = {
    "WILD": 38,
    "TRAINER": 30,
    "ELITE": 8,
    "SHOP": 8,
    "REST": 8,
    "EVENT": 8,
}

MAP_POOLS = {
    "FOREST": [(0, 0), (0, 1), (0, 2), (0, 3)],
    "MOUNTAIN": [(0, 10), (0, 11), (0, 12), (0, 13)],
    "RUINS": [(0, 20), (0, 21), (0, 22), (0, 23)],
}

SPECIES_POOLS = {
    "FOREST": [16, 19, 25, 261, 263, 285],
    "MOUNTAIN": [66, 74, 95, 299, 304, 307],
    "RUINS": [92, 104, 200, 343, 344, 355],
}


@dataclass
class Node:
    index: int
    node_type: str
    danger: int
    reward_tier: int
    map_group: int
    map_num: int


@dataclass
class EnemyMon:
    species: int
    level: int


@dataclass
class EnemyTeam:
    party_size: int
    ai_level: int
    party: List[EnemyMon]


@dataclass
class BattlePlan:
    encounter_type: int
    ai_level: int
    reward_tier: int
    gives_heal: int
    gives_shop: int


@dataclass
class NodeResolution:
    selected_node: int
    reward_tier: int
    gold_reward: int
    healed: int
    opens_shop: int
    encounter_type: int


@dataclass
class HubState:
    hub_currency: int
    upgrade_level: int
    unlocked_team_size: int
    medals: int


@dataclass
class GymProgress:
    is_gym_leader: int
    medal_awarded: int
    new_medal_count: int


@dataclass
class Floor:
    floor: int
    biome: str
    nodes: List[Node]
    zone_choices: List[dict]


class LCG:
    def __init__(self, seed: int):
        self.state = seed & 0xFFFFFFFF

    def next_u32(self) -> int:
        self.state = (1664525 * self.state + 1013904223) & 0xFFFFFFFF
        return self.state

    def randrange(self, n: int) -> int:
        if n <= 0:
            raise ValueError("n debe ser > 0")
        return self.next_u32() % n


def weighted_pick(rng: LCG, weights: Dict[str, int]) -> str:
    total = sum(weights.values())
    roll = rng.randrange(total)
    acc = 0
    for k, w in weights.items():
        acc += w
        if roll < acc:
            return k
    return "WILD"


def biome_for_floor(floor_index: int) -> str:
    if floor_index < 8:
        return "FOREST"
    if floor_index < 16:
        return "MOUNTAIN"
    return "RUINS"


def rng_for_floor(seed: int, floor_index: int) -> LCG:
    rng = LCG(seed)
    for _ in range(floor_index + 1):
        rng.next_u32()
    return rng


def zone_choices(seed: int, floor_index: int, count: int = 2) -> List[dict]:
    rng = LCG(seed ^ 0xA5A5A5A5)
    for _ in range(floor_index + 1):
        rng.next_u32()

    biome = biome_for_floor(floor_index + 1)
    pool = MAP_POOLS[biome]
    used = set()
    out = []
    for _ in range(count):
        idx = rng.randrange(len(pool))
        tries = 0
        while idx in used and tries < 8:
            idx = rng.randrange(len(pool))
            tries += 1
        used.add(idx)
        grp, num = pool[idx]
        out.append({"map_group": grp, "map_num": num})
    return out


def generate_enemy_team(seed: int, floor_index: int, player_avg_level: int, player_party_size: int, is_boss: bool) -> EnemyTeam:
    rng = LCG(seed ^ 0x3C3C3C3C)
    for _ in range(floor_index + 1):
        rng.next_u32()

    biome = biome_for_floor(floor_index)
    species_pool = SPECIES_POOLS[biome]
    base_party = 2 + floor_index // 3
    if is_boss:
        base_party += 2

    party_size = max(1, min(6, max(base_party, player_party_size)))

    if is_boss:
        ai_level = 3
    elif floor_index >= 12:
        ai_level = 2
    elif floor_index >= 4:
        ai_level = 1
    else:
        ai_level = 0

    party: List[EnemyMon] = []
    for _ in range(party_size):
        level = max(3, player_avg_level - 3)
        level += rng.randrange(7)
        level += floor_index // 2
        if is_boss:
            level += 2
        level = max(2, min(100, level))
        species = species_pool[rng.randrange(len(species_pool))]
        party.append(EnemyMon(species=species, level=level))

    return EnemyTeam(party_size=party_size, ai_level=ai_level, party=party)


def build_battle_plan(node: Node, team: EnemyTeam) -> BattlePlan:
    ai_level = team.ai_level
    reward_tier = node.reward_tier
    gives_heal = 0
    gives_shop = 0

    if node.node_type == "WILD":
        encounter_type = 0
    elif node.node_type == "TRAINER":
        encounter_type = 1
    elif node.node_type == "ELITE":
        encounter_type = 2
        ai_level = 2
    elif node.node_type == "BOSS":
        encounter_type = 3
        ai_level = 3
        reward_tier = 5
    elif node.node_type == "SHOP":
        encounter_type = 4
        gives_shop = 1
    elif node.node_type == "REST":
        encounter_type = 4
        gives_heal = 1
    else:
        encounter_type = 5

    return BattlePlan(
        encounter_type=encounter_type,
        ai_level=ai_level,
        reward_tier=reward_tier,
        gives_heal=gives_heal,
        gives_shop=gives_shop,
    )


def resolve_node(seed: int, current_floor: int, floor: Floor, node_index: int, plan: BattlePlan) -> NodeResolution:
    if node_index >= len(floor.nodes):
        node_index = 0

    rng = LCG(seed ^ 0x5F3759DF)
    for _ in range(current_floor + node_index + 1):
        rng.next_u32()

    base_gold = 80 + plan.reward_tier * 40 + current_floor * 12
    gold = base_gold + rng.randrange(51)

    return NodeResolution(
        selected_node=node_index,
        reward_tier=plan.reward_tier,
        gold_reward=gold,
        healed=plan.gives_heal,
        opens_shop=plan.gives_shop,
        encounter_type=plan.encounter_type,
    )


def advance_run(current_floor: int, total_floors: int) -> int:
    if current_floor < total_floors:
        return current_floor + 1
    return current_floor


def run_is_finished(current_floor: int, total_floors: int) -> bool:
    return current_floor >= total_floors


def hub_init() -> HubState:
    return HubState(hub_currency=0, upgrade_level=0, unlocked_team_size=3, medals=0)


def hub_apply_resolution(hub: HubState, resolution: NodeResolution) -> None:
    hub.hub_currency += resolution.gold_reward
    if resolution.healed:
        hub.hub_currency += 25
    if resolution.opens_shop:
        hub.hub_currency += 15


def hub_upgrade_team_size(hub: HubState) -> None:
    cost = 180 + hub.unlocked_team_size * 40
    if hub.unlocked_team_size >= 6:
        return
    if hub.hub_currency >= cost:
        hub.hub_currency -= cost
        hub.unlocked_team_size += 1


def hub_upgrade_power(hub: HubState) -> None:
    cost = 160 + hub.upgrade_level * 60
    if hub.upgrade_level >= 10:
        return
    if hub.hub_currency >= cost:
        hub.hub_currency -= cost
        hub.upgrade_level += 1


def evaluate_gym_leader(current_floor: int, plan: BattlePlan, hub: HubState) -> GymProgress:
    is_gym = 0
    medal_awarded = 0

    if (
        hub.medals < 8
        and current_floor > 0
        and current_floor % 4 == 0
        and plan.encounter_type in {1, 2, 3}
    ):
        is_gym = 1
        medal_awarded = 1
        hub.medals += 1

    return GymProgress(is_gym_leader=is_gym, medal_awarded=medal_awarded, new_medal_count=hub.medals)


def generate_floor(seed: int, floor_index: int, total_floors: int) -> Floor:
    rng = rng_for_floor(seed, floor_index)
    weights = dict(NODE_WEIGHTS_BASE)

    if floor_index == 0:
        weights["ELITE"] = 0

    if floor_index > 0 and floor_index % 4 == 0:
        weights["SHOP"] += 6
        weights["REST"] += 6

    node_count = 3 + rng.randrange(3)
    nodes: List[Node] = []
    pool = MAP_POOLS[biome_for_floor(floor_index)]

    forced_support_inserted = False

    for i in range(node_count):
        t = weighted_pick(rng, weights)

        if floor_index > 0 and floor_index % 4 == 0 and i == node_count - 1 and not forced_support_inserted:
            t = "SHOP" if rng.randrange(2) == 0 else "REST"
            forced_support_inserted = True

        if floor_index == total_floors - 1 and i == node_count - 1:
            t = "BOSS"

        danger = 1 + floor_index + (1 if t in {"ELITE", "BOSS"} else 0)
        reward_tier = min(5, 1 + floor_index // 3 + (1 if t in {"ELITE", "BOSS"} else 0))
        map_group, map_num = pool[rng.randrange(len(pool))]
        nodes.append(
            Node(
                index=i,
                node_type=t,
                danger=danger,
                reward_tier=reward_tier,
                map_group=map_group,
                map_num=map_num,
            )
        )

    return Floor(
        floor=floor_index + 1,
        biome=biome_for_floor(floor_index),
        nodes=nodes,
        zone_choices=zone_choices(seed, floor_index),
    )


def generate_run(seed: int, floors: int, player_avg_level: int = 15, player_party_size: int = 3) -> dict:
    if floors < 1:
        raise ValueError("floors debe ser >= 1")

    route = []
    hub = hub_init()

    for i in range(floors):
        floor = generate_floor(seed, i, floors)
        is_boss = i == floors - 1
        enemy_team = generate_enemy_team(seed, i, player_avg_level + hub.upgrade_level, min(6, max(player_party_size, hub.unlocked_team_size)), is_boss)
        battle_plans = [build_battle_plan(n, enemy_team) for n in floor.nodes]
        resolution = resolve_node(seed, i, floor, 0, battle_plans[0])
        gym = evaluate_gym_leader(i, battle_plans[0], hub)
        hub_apply_resolution(hub, resolution)
        hub_upgrade_power(hub)
        hub_upgrade_team_size(hub)

        route.append(
            {
                "floor": floor.floor,
                "biome": floor.biome,
                "nodes": [asdict(n) for n in floor.nodes],
                "zone_choices": floor.zone_choices,
                "enemy_team": {
                    "party_size": enemy_team.party_size,
                    "ai_level": enemy_team.ai_level,
                    "party": [asdict(mon) for mon in enemy_team.party],
                },
                "battle_plans": [asdict(bp) for bp in battle_plans],
                "sample_resolution": asdict(resolution),
                "gym_progress": asdict(gym),
                "hub_state": asdict(hub),
                "next_floor": advance_run(i, floors),
                "run_finished": run_is_finished(advance_run(i, floors), floors),
            }
        )

    return {
        "seed": seed,
        "floors": floors,
        "player_avg_level": player_avg_level,
        "player_party_size": player_party_size,
        "route": route,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Generador de run roguelike determinista")
    parser.add_argument("--seed", type=int, required=True, help="Semilla de 32 bits")
    parser.add_argument("--floors", type=int, default=8, help="Cantidad de pisos")
    parser.add_argument("--player-avg-level", type=int, default=15, help="Nivel promedio del equipo del jugador")
    parser.add_argument("--player-party-size", type=int, default=3, help="Cantidad de Pokémon en equipo del jugador")
    parser.add_argument("--output", type=str, default="", help="Archivo JSON de salida")
    args = parser.parse_args()

    run_data = generate_run(args.seed, args.floors, args.player_avg_level, args.player_party_size)
    payload = json.dumps(run_data, indent=2, ensure_ascii=False)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as fh:
            fh.write(payload + "\n")
        print(f"Run generada en {args.output}")
    else:
        print(payload)


if __name__ == "__main__":
    main()
