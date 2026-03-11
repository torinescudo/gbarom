import importlib.util
import json
import subprocess
import sys
from pathlib import Path

SPEC = importlib.util.spec_from_file_location("seed_generator", Path("tools/seed_generator.py"))
seed_generator = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
sys.modules[SPEC.name] = seed_generator
SPEC.loader.exec_module(seed_generator)


def run(seed: int, floors: int):
    out = subprocess.check_output(
        ["python3", "tools/seed_generator.py", "--seed", str(seed), "--floors", str(floors)],
        text=True,
    )
    return json.loads(out)


def test_deterministic_same_seed():
    a = run(42, 6)
    b = run(42, 6)
    assert a == b


def test_last_node_is_boss():
    data = run(7, 5)
    last_floor = data["route"][-1]
    assert last_floor["nodes"][-1]["node_type"] == "BOSS"


def test_floor_generation_is_stable_per_floor_index():
    a = seed_generator.generate_floor(seed=1234, floor_index=4, total_floors=8)
    b = seed_generator.generate_floor(seed=1234, floor_index=4, total_floors=8)
    assert a == b


def test_enemy_team_adapts_to_player_level():
    low = seed_generator.generate_enemy_team(1234, floor_index=3, player_avg_level=10, player_party_size=3, is_boss=False)
    high = seed_generator.generate_enemy_team(1234, floor_index=3, player_avg_level=35, player_party_size=3, is_boss=False)
    assert max(mon.level for mon in high.party) > max(mon.level for mon in low.party)


def test_file_output(tmp_path: Path):
    target = tmp_path / "run.json"
    subprocess.check_call(
        [
            "python3",
            "tools/seed_generator.py",
            "--seed",
            "99",
            "--floors",
            "4",
            "--output",
            str(target),
        ]
    )
    assert target.exists()
    loaded = json.loads(target.read_text(encoding="utf-8"))
    assert loaded["seed"] == 99


def test_zone_choices_present_and_count():
    f = seed_generator.generate_floor(seed=1234, floor_index=2, total_floors=8)
    assert len(f.zone_choices) == 2


def test_battle_plan_support_nodes_flags():
    n = seed_generator.Node(index=0, node_type="REST", danger=1, reward_tier=1, map_group=0, map_num=0)
    t = seed_generator.EnemyTeam(party_size=3, ai_level=1, party=[])
    bp = seed_generator.build_battle_plan(n, t)
    assert bp.gives_heal == 1
    assert bp.gives_shop == 0


def test_resolve_node_and_run_progression():
    floor = seed_generator.generate_floor(seed=77, floor_index=1, total_floors=4)
    team = seed_generator.generate_enemy_team(77, floor_index=1, player_avg_level=20, player_party_size=3, is_boss=False)
    plan = seed_generator.build_battle_plan(floor.nodes[0], team)
    res = seed_generator.resolve_node(77, current_floor=1, floor=floor, node_index=0, plan=plan)
    assert res.gold_reward >= 80
    next_floor = seed_generator.advance_run(1, 4)
    assert next_floor == 2
    assert seed_generator.run_is_finished(next_floor, 4) is False


def test_hub_progression_and_medal_award():
    hub = seed_generator.hub_init()
    floor = seed_generator.generate_floor(seed=12, floor_index=4, total_floors=8)
    team = seed_generator.generate_enemy_team(12, floor_index=4, player_avg_level=20, player_party_size=3, is_boss=False)
    node = seed_generator.Node(index=0, node_type="TRAINER", danger=2, reward_tier=2, map_group=0, map_num=1)
    plan = seed_generator.build_battle_plan(node, team)
    gym = seed_generator.evaluate_gym_leader(4, plan, hub)
    assert gym.is_gym_leader == 1
    assert gym.medal_awarded == 1
    res = seed_generator.resolve_node(12, current_floor=4, floor=floor, node_index=0, plan=plan)
    seed_generator.hub_apply_resolution(hub, res)
    assert hub.hub_currency > 0


def test_begin_run_and_save_load_roundtrip():
    hub = seed_generator.hub_init()
    start = seed_generator.StartConfig(seed=999, total_floors=10, starting_team_size=4, level_bonus=2)
    run = seed_generator.begin_run_from_hub(start, hub)
    assert run["seed"] == 999
    assert hub.unlocked_team_size >= 4
    assert hub.upgrade_level == 2
    data = seed_generator.save_to_data(run, hub)
    loaded_run, loaded_hub = seed_generator.load_from_data(data)
    assert loaded_run == run
    assert loaded_hub.unlocked_team_size == hub.unlocked_team_size


def test_runtime_context_flow():
    start = seed_generator.StartConfig(seed=123, total_floors=5, starting_team_size=3, level_bonus=1)
    ctx = seed_generator.runtime_begin_new_run(start)
    seed_generator.runtime_build_current_floor(ctx, player_avg_level=18, player_party_size=3)
    res, gym = seed_generator.runtime_select_node(ctx, 0)
    assert res.gold_reward >= 80
    _ = gym
    save = seed_generator.runtime_save(ctx)
    assert save.current_floor == 1


def test_runtime_auto_step_progresses():
    start = seed_generator.StartConfig(seed=321, total_floors=4, starting_team_size=3, level_bonus=0)
    ctx = seed_generator.runtime_begin_new_run(start)
    _, _, picked = seed_generator.runtime_play_step(ctx, player_avg_level=16, player_party_size=3)
    assert picked >= 0
    assert ctx.run_state["current_floor"] == 1
