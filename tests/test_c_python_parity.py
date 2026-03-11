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
            "integration/pokeemerald/src/roguelike_runtime.c",
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
    start = seed_generator.StartConfig(seed=seed, total_floors=total_floors, starting_team_size=3, level_bonus=0)
    ctx = seed_generator.runtime_begin_new_run(start)
    ctx.run_state["current_floor"] = floor_index
    seed_generator.runtime_build_current_floor(ctx, player_avg_level=15, player_party_size=3)
    picked = seed_generator.runtime_auto_pick_node(ctx)
    resolution, gym = seed_generator.runtime_select_node(ctx, picked)
    save_data = seed_generator.runtime_save(ctx)

    return {
        "floor": ctx.floor.floor,
        "biome": {"FOREST": 0, "MOUNTAIN": 1, "RUINS": 2}[ctx.floor.biome],
        "nodes": [
            {
                "index": n.index,
                "node_type": n.node_type,
                "danger": n.danger,
                "reward_tier": n.reward_tier,
                "map_group": n.map_group,
                "map_num": n.map_num,
            }
            for n in ctx.floor.nodes
        ],
        "zone_choices": ctx.zone_choices,
        "battle_plans": [
            {
                "encounter_type": bp.encounter_type,
                "ai_level": bp.ai_level,
                "reward_tier": bp.reward_tier,
                "gives_heal": bp.gives_heal,
                "gives_shop": bp.gives_shop,
            }
            for bp in ctx.battle_plans
        ],
        "enemy_team": {
            "party_size": ctx.enemy_team.party_size,
            "ai_level": ctx.enemy_team.ai_level,
            "party": [{"species": m.species, "level": m.level} for m in ctx.enemy_team.party],
        },
        "selected_node": picked,
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
            "hub_currency": ctx.hub.hub_currency,
            "upgrade_level": ctx.hub.upgrade_level,
            "unlocked_team_size": ctx.hub.unlocked_team_size,
            "medals": ctx.hub.medals,
        },
        "save_data": {
            "seed": save_data.seed,
            "total_floors": save_data.total_floors,
            "current_floor": save_data.current_floor,
            "hub_currency": save_data.hub_currency,
            "hub_upgrade_level": save_data.hub_upgrade_level,
            "hub_unlocked_team_size": save_data.hub_unlocked_team_size,
            "hub_medals": save_data.hub_medals,
        },
        "next_floor": ctx.run_state["current_floor"],
        "run_finished": 1 if seed_generator.run_is_finished(ctx.run_state["current_floor"], total_floors) else 0,
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
