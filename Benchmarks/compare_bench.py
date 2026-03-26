#!/usr/bin/env python3
"""
Simple benchmark comparator for Chemical Simulator.

Runs two benchmark groups in one launch and prints:
- baseline time
- candidate time
- delta in %
- speedup (x)
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

BINARY_NAME = "benchmarks" if sys.platform != "win32" else "benchmarks.exe"
BINARY = Path(__file__).parent.parent / BINARY_NAME
USE_COLOR = sys.stdout.isatty()

GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
RESET = "\033[0m"


def die(msg: str) -> None:
    print(f"Error: {msg}", file=sys.stderr)
    sys.exit(1)


def list_groups() -> list[str]:
    if not BINARY.exists():
        die(f"Benchmark binary not found: {BINARY}")

    result = subprocess.run(
        [str(BINARY), "--benchmark_list_tests=true"],
        capture_output=True,
        text=True,
        check=False,
    )
    names = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    groups: dict[str, None] = {}
    for name in names:
        parts = name.split("/")
        group = "/".join(parts[:2]) if len(parts) >= 2 else parts[0]
        groups[group] = None
    return list(groups.keys())


def colorize(text: str, color: str) -> str:
    if not USE_COLOR:
        return text
    return f"{color}{text}{RESET}"


def choose_groups_pair(groups: list[str]) -> tuple[str, str]:
    print("Choose baseline and candidate by numbers (e.g. 3 4):")
    for i, group in enumerate(groups, 1):
        print(f"  {i}) {group}")
    choice = input("Choose two numbers: ").strip()
    parts = choice.split()
    if len(parts) != 2 or not all(p.isdigit() for p in parts):
        die("Expected two numbers: <baseline> <candidate>")
    i1 = int(parts[0]) - 1
    i2 = int(parts[1]) - 1
    if i1 < 0 or i1 >= len(groups) or i2 < 0 or i2 >= len(groups):
        die("Invalid index")
    if i1 == i2:
        die("Baseline and candidate must be different")
    return groups[i1], groups[i2]


def run_json(filter_regex: str, repetitions: int) -> dict:
    cmd = [
        str(BINARY),
        "--benchmark_format=json",
        f"--benchmark_repetitions={repetitions}",
        f"--benchmark_filter={filter_regex}",
    ]
    print("$ " + " ".join(cmd))
    result = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if result.stderr:
        print(result.stderr)
    if result.returncode != 0:
        die(f"Benchmark exited with code {result.returncode}")
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError as e:
        die(f"Failed to parse benchmark JSON: {e}")


def pick_metric(entry: dict) -> float:
    # Prefer CPU time for stability in local runs.
    return float(entry.get("cpu_time", entry.get("real_time", 0.0)))


def extract_user_counters(entry: dict) -> dict[str, float]:
    counters: dict[str, float] = {}
    for key, value in entry.items():
        if key.startswith("nl_") or key.startswith("step_"):
            try:
                counters[key] = float(value)
            except (TypeError, ValueError):
                continue
    return counters


def extract_group_points(data: dict, group_name: str) -> tuple[dict[int, float], dict[int, dict[str, float]]]:
    points: dict[int, float] = {}
    counters_by_size: dict[int, dict[str, float]] = {}
    prefix = group_name + "/"
    for row in data.get("benchmarks", []):
        name = row.get("name", "")
        if not name.startswith(prefix):
            continue

        # For repeated runs, prefer median aggregate rows.
        aggregate = row.get("aggregate_name")
        if aggregate is not None and aggregate != "median":
            continue

        m = re.match(rf"^{re.escape(group_name)}/(\d+)", name)
        if not m:
            continue
        size = int(m.group(1))
        points[size] = pick_metric(row)
        counters_by_size[size] = extract_user_counters(row)
    return points, counters_by_size


def print_counter_comparison(
    base_name: str,
    cand_name: str,
    base_counters: dict[int, dict[str, float]],
    cand_counters: dict[int, dict[str, float]],
) -> None:
    common_sizes = sorted(set(base_counters.keys()) & set(cand_counters.keys()))
    if not common_sizes:
        return

    all_keys: list[str] = []
    for size in common_sizes:
        keys = set(base_counters.get(size, {}).keys()) | set(cand_counters.get(size, {}).keys())
        all_keys.extend(keys)
    keys_ordered = sorted(set(all_keys))
    if not keys_ordered:
        return

    print()
    print("NeighborList counters:")
    print(f"(base={base_name}, candidate={cand_name})")
    print()
    for size in common_sizes:
        print(f"  Size {size}:")
        for key in keys_ordered:
            b = base_counters.get(size, {}).get(key)
            c = cand_counters.get(size, {}).get(key)
            if b is None and c is None:
                continue
            b_str = "-" if b is None else f"{b:.6g}"
            c_str = "-" if c is None else f"{c:.6g}"
            print(f"    {key}: base={b_str}, candidate={c_str}")


def print_comparison(base_name: str, cand_name: str, base: dict[int, float], cand: dict[int, float]) -> None:
    common_sizes = sorted(set(base.keys()) & set(cand.keys()))
    if not common_sizes:
        die("No overlapping sizes between baseline and candidate.")

    print()
    print(f"Baseline : {base_name}")
    print(f"Candidate: {cand_name}")
    print()
    print(f"{'Size':>8} | {'Base (ns)':>12} | {'Cand (ns)':>12} | {'Delta %':>9} | {'Speedup':>8}")
    print("-" * 64)

    for size in common_sizes:
        b = base[size]
        c = cand[size]
        delta_pct = ((c - b) / b) * 100.0 if b != 0 else 0.0
        speedup = (b / c) if c != 0 else 0.0
        delta_text = f"{delta_pct:>8.2f}%"
        speedup_text = f"{speedup:>7.2f}x"
        if delta_pct < 0:
            delta_text = colorize(delta_text, GREEN)
            speedup_text = colorize(speedup_text, GREEN)
        elif delta_pct > 0:
            delta_text = colorize(delta_text, RED)
            speedup_text = colorize(speedup_text, RED)
        else:
            delta_text = colorize(delta_text, YELLOW)
            speedup_text = colorize(speedup_text, YELLOW)
        print(f"{size:>8} | {b:>12.1f} | {c:>12.1f} | {delta_text} | {speedup_text}")

    avg_delta = sum(((cand[s] - base[s]) / base[s]) * 100.0 for s in common_sizes if base[s] != 0) / len(common_sizes)
    print("-" * 64)
    verdict = "better" if avg_delta < 0 else "worse"
    avg_text = f"{avg_delta:+.2f}%"
    if avg_delta < 0:
        avg_text = colorize(avg_text, GREEN)
        verdict = colorize(verdict, GREEN)
    elif avg_delta > 0:
        avg_text = colorize(avg_text, RED)
        verdict = colorize(verdict, RED)
    else:
        avg_text = colorize(avg_text, YELLOW)
        verdict = colorize(verdict, YELLOW)
    print(f"Average delta: {avg_text} -> candidate is {verdict}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Compare two benchmark groups in console")
    parser.add_argument("--base", help="Baseline benchmark group, e.g. SimulationFixture/ComputeForcesNoNeighborList")
    parser.add_argument("--candidate", help="Candidate benchmark group, e.g. SimulationFixture/ComputeForcesWithNeighborList")
    parser.add_argument("--repetitions", type=int, default=3, help="Benchmark repetitions (default: 3)")
    args = parser.parse_args()

    if args.base and args.candidate:
        base_group = args.base
        candidate_group = args.candidate
    else:
        groups = list_groups()
        print("Interactive compare mode")
        base_group, candidate_group = choose_groups_pair(groups)

    filter_regex = "(" + "|".join(re.escape(x) for x in (base_group, candidate_group)) + ")"
    data = run_json(filter_regex, args.repetitions)

    base_points, base_counters = extract_group_points(data, base_group)
    cand_points, cand_counters = extract_group_points(data, candidate_group)
    print_comparison(base_group, candidate_group, base_points, cand_points)
    print_counter_comparison(base_group, candidate_group, base_counters, cand_counters)


if __name__ == "__main__":
    main()
