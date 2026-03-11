import json
import subprocess
from pathlib import Path

import importlib.util
import sys

SPEC = importlib.util.spec_from_file_location("seed_generator", Path("tools/seed_generator.py"))
seed_generator = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
sys.modules[SPEC.name] = seed_generator
SPEC.loader.exec_module(seed_generator)


def build_harness(binary: Path) -> None:
    subprocess.check_call(
        [
            "gcc",
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-Itests/c_harness",
            "-Iintegration/pokeemerald/include",
            "integration/pokeemerald/src/roguelike.c",
            "tests/c_harness/rogue_dump.c",
            "-o",
            str(binary),
        ]
    )


def c_floor(binary: Path, seed: int, floor_index: int, total_floors: int) -> dict:
    out = subprocess.check_output(
        [str(binary), str(seed), str(floor_index), str(total_floors)],
        text=True,
    )
    return json.loads(out)


def normalize_py_floor(seed: int, floor_index: int, total_floors: int) -> dict:
    hub = seed_generator.hub_init()
    f = seed_generator.generate_floor(seed=seed, floor_index=floor_index, total_floors=total_floors)
    team = seed_generator.generate_enemy_team(
        seed=seed,
        floor_index=floor_index,
        player_avg_level=15 + hub.upgrade_level,
        player_party_size=hub.unlocked_team_size,
        is_boss=floor_index == total_floors - 1,
    )
    plans = [seed_generator.build_battle_plan(n, team) for n in f.nodes]
    resolution = seed_generator.resolve_node(seed, floor_index, f, 0, plans[0])
    gym = seed_generator.evaluate_gym_leader(floor_index, plans[0], hub)
    seed_generator.hub_apply_resolution(hub, resolution)
    seed_generator.hub_upgrade_power(hub)
    seed_generator.hub_upgrade_team_size(hub)
    next_floor = seed_generator.advance_run(floor_index, total_floors)

    return {
        "floor": f.floor,
        "biome": {"FOREST": 0, "MOUNTAIN": 1, "RUINS": 2}[f.biome],
        "nodes": [
            {
                "index": n.index,
                "node_type": n.node_type,
                "danger": n.danger,
                "reward_tier": n.reward_tier,
                "map_group": n.map_group,
                "map_num": n.map_num,
            }
            for n in f.nodes
        ],
        "zone_choices": f.zone_choices,
        "battle_plans": [
            {
                "encounter_type": bp.encounter_type,
                "ai_level": bp.ai_level,
                "reward_tier": bp.reward_tier,
                "gives_heal": bp.gives_heal,
                "gives_shop": bp.gives_shop,
            }
            for bp in plans
        ],
        "enemy_team": {
            "party_size": team.party_size,
            "ai_level": team.ai_level,
            "party": [{"species": m.species, "level": m.level} for m in team.party],
        },
        "sample_resolution": {
            "selected_node": resolution.selected_node,
            "reward_tier": resolution.reward_tier,
            "gold_reward": resolution.gold_reward,
            "healed": resolution.healed,
            "opens_shop": resolution.opens_shop,
            "encounter_type": resolution.encounter_type,
        },
        "gym_progress": {
            "is_gym_leader": gym.is_gym_leader,
            "medal_awarded": gym.medal_awarded,
            "new_medal_count": gym.new_medal_count,
        },
        "hub_state": {
            "hub_currency": hub.hub_currency,
            "upgrade_level": hub.upgrade_level,
            "unlocked_team_size": hub.unlocked_team_size,
            "medals": hub.medals,
        },
        "next_floor": next_floor,
        "run_finished": 1 if seed_generator.run_is_finished(next_floor, total_floors) else 0,
    }


def test_c_and_python_floor_parity(tmp_path: Path):
    binary = tmp_path / "rogue_dump"
    build_harness(binary)

    scenarios = [
        (1, 0, 8),
        (1337, 4, 8),
        (1337, 7, 8),
        (987654, 11, 16),
    ]

    for seed, floor_index, total_floors in scenarios:
        c_data = c_floor(binary, seed, floor_index, total_floors)
        py_data = normalize_py_floor(seed, floor_index, total_floors)
        assert c_data == py_data
