#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
bench.py — запускалка бенчмарков для Chemical Simulator.

Использование:
  ./bench.py                          — интерактивное меню
  ./bench.py --filter SimulationFixture
  ./bench.py --filter Correct --save
  ./bench.py --list                   — показать сохранённые результаты
  ./bench.py --open                   — открыть view.html в браузере
"""

SCENES: list[tuple[str, str]] = [
    ("crystal3d", "Кристалл 3D"),
    ("crystal2d", "Кристалл 2D"),
    ("random_gas2d", "Случайный газ 2D"),
]

import argparse
import json
import math
import os
import re
import subprocess
import sys
import time
import webbrowser
from datetime import datetime
from pathlib import Path

BINARY_NAME = "benchmarks" if sys.platform != "win32" else "benchmarks.exe"
BINARY = Path(__file__).parent.parent / BINARY_NAME
RESULTS_DIR = Path(__file__).parent / "results"
VIEW_HTML = Path(__file__).parent / "view.html"
BENCHMARKS_ROOT = Path(__file__).parent

COLOR_RESET = "\033[0m"
COLOR_TITLE = "\033[1;97m"
COLOR_GROUP = "\033[92m"
COLOR_SUBGROUP = "\033[95m"
COLOR_SUBGROUP_BRACKET = "\033[90m"
COLOR_INDEX = "\033[33m"
COLOR_INDEX_LIGHT_BLUE = "\033[96m"
COLOR_HINT = "\033[90m"
COLOR_ERROR = "\033[31m"
COLOR_OK = "\033[32m"
COLOR_WARN = "\033[33m"
COLOR_BAD = "\033[31m"
COLOR_UNIT_NS = "\033[90m"
COLOR_UNIT_US = "\033[36m"
COLOR_UNIT_MS = "\033[35m"
COLOR_PROGRESS_SPINNER = "\033[96m"
COLOR_PROGRESS_BAR = "\033[36m"
COLOR_PROGRESS_COUNT = "\033[33m"
COLOR_PROGRESS_CASE = "\033[95m"
COLOR_PROGRESS_TIME = "\033[92m"

def paint(text: str, color: str) -> str:
    if not sys.stdout.isatty():
        return text
    return f"{color}{text}{COLOR_RESET}"


def paint_subgroup_label(text: str) -> str:
    if not sys.stdout.isatty():
        return f"[{text}]"
    return (
        f"{COLOR_SUBGROUP_BRACKET}[{COLOR_RESET}"
        f"{COLOR_SUBGROUP}{text}{COLOR_RESET}"
        f"{COLOR_SUBGROUP_BRACKET}]{COLOR_RESET}"
    )


def paint_256(text: str, color_code: int) -> str:
    if not sys.stdout.isatty():
        return text
    return f"\033[38;5;{color_code}m{text}{COLOR_RESET}"


def paint_rgb(text: str, r: int, g: int, b: int) -> str:
    if not sys.stdout.isatty():
        return text
    r = max(0, min(255, r))
    g = max(0, min(255, g))
    b = max(0, min(255, b))
    return f"\033[38;2;{r};{g};{b}m{text}{COLOR_RESET}"


def short_bench_id(full_name: str) -> str:
    parts = full_name.split("/")
    return "/".join(parts[:2]) if len(parts) >= 2 else full_name


def bench_arg(full_name: str) -> str:
    parts = full_name.split("/")
    return parts[2] if len(parts) >= 3 else "-"


def fmt_num(value: float | None, digits: int = 2) -> str:
    if value is None:
        return "-"
    return f"{value:.{digits}f}"

def fmt_cv(value: float | None) -> str:
    if value is None:
        return "-"
    text = f"{value:.2f}"
    if value >= 5.0:
        return f"{text} !"
    return text


def fmt_time_ns(value: float | None) -> str:
    if value is None:
        return "-"
    abs_v = abs(value)
    if abs_v >= 1_000_000.0:
        return f"{value / 1_000_000.0:.2f} ms"
    if abs_v >= 1_000.0:
        return f"{value / 1_000.0:.2f} us"
    return f"{value:.2f} ns"


def fmt_ips(value: float | None) -> str:
    if value is None:
        return "-"
    abs_v = abs(value)
    if abs_v >= 1_000_000_000:
        return f"{value / 1_000_000_000:.2f}G"
    if abs_v >= 1_000_000:
        return f"{value / 1_000_000:.2f}M"
    if abs_v >= 1_000:
        return f"{value / 1_000:.2f}k"
    return f"{value:.2f}"


def paint_numeric(cell: str, cv_mode: bool = False) -> str:
    if cell.strip() == "-":
        return cell
    if cv_mode:
        try:
            m = re.search(r"[-+]?\d*\.?\d+", cell.strip())
            if not m:
                return paint(cell, COLOR_INDEX)
            cv = float(m.group(0))
        except ValueError:
            return paint(cell, COLOR_INDEX)
        if cv < 2.0:
            return paint(cell, COLOR_OK)
        if cv < 5.0:
            return paint(cell, COLOR_WARN)
        return paint(cell, COLOR_BAD)
    return paint(cell, COLOR_INDEX)

def paint_delta(cell: str, neutral_threshold: float = 2.5) -> str:
    value = cell.strip()
    if value == "-" or not value:
        return cell
    try:
        delta = float(value.replace("%", ""))
    except ValueError:
        return cell
    if delta <= -neutral_threshold:
        return paint(cell, COLOR_OK)    # быстрее / лучше
    if delta >= neutral_threshold:
        return paint(cell, COLOR_BAD)   # медленнее / хуже
    return paint(cell, COLOR_HINT)      # нейтрально


def paint_n_gradient(cell: str, min_n: int, max_n: int) -> str:
    raw = cell.strip()
    if not raw.isdigit():
        return cell
    n = int(raw)
    if max_n <= min_n:
        return paint_256(cell, 81)
    t = (n - min_n) / float(max_n - min_n)
    # Мягкий монотонный градиент: голубой -> лавандовый.
    r = int(120 + (170 - 120) * t)
    g = int(200 + (165 - 200) * t)
    b = int(255 + (245 - 255) * t)
    return paint_rgb(cell, r, g, b)


def paint_time_cell(cell: str) -> str:
    if not sys.stdout.isatty():
        return cell

    value = cell.rstrip()
    tail_spaces = cell[len(value):]
    if value == "-":
        return cell

    if value.endswith(" ns"):
        return value[:-3] + " " + paint("ns", COLOR_UNIT_NS) + tail_spaces
    if value.endswith(" us"):
        return value[:-3] + " " + paint("us", COLOR_UNIT_US) + tail_spaces
    if value.endswith(" ms"):
        return value[:-3] + " " + paint("ms", COLOR_UNIT_MS) + tail_spaces
    return cell


def paint_complexity_label(label_cell: str) -> str:
    raw = label_cell.strip()
    if raw == "O(1)":
        return paint(label_cell, COLOR_OK)
    if raw == "O(N)":
        return paint(label_cell, "\033[34m")
    if raw == "O(log N)":
        return paint(label_cell, COLOR_INDEX_LIGHT_BLUE)
    if raw == "O(N log N)":
        return paint(label_cell, COLOR_WARN)
    if raw.startswith("O(N^") or raw == "O(N^k)":
        return paint(label_cell, COLOR_BAD)
    return label_cell


def paint_points_count(points_cell: str) -> str:
    raw = points_cell.strip()
    if not raw.isdigit():
        return points_cell
    count = int(raw)
    # 3 точки: минимально допустимо, но ненадежно.
    if count <= 3:
        marked = points_cell.rstrip() + " (!)"
        return paint(marked, COLOR_BAD)
    if count <= 5:
        return paint(points_cell, COLOR_WARN)
    return paint(points_cell, COLOR_OK)


def parse_benchmark_rows(data: dict) -> dict[str, dict[str, float | str]]:
    benchmarks = data.get("benchmarks", [])
    rows: dict[str, dict[str, float | str]] = {}

    for item in benchmarks:
        run_name = item.get("run_name") or item.get("name")
        if not isinstance(run_name, str):
            continue
        row = rows.setdefault(run_name, {})
        run_type = item.get("run_type")

        if run_type == "aggregate":
            agg = item.get("aggregate_name")
            if agg == "median":
                row["real_median"] = float(item.get("real_time", 0.0))
                row["cpu_median"] = float(item.get("cpu_time", 0.0))
                if "items_per_second" in item:
                    row["ips"] = float(item.get("items_per_second", 0.0))
            elif agg == "mean":
                row["real_mean"] = float(item.get("real_time", 0.0))
                if "items_per_second" in item and "ips" not in row:
                    row["ips"] = float(item.get("items_per_second", 0.0))
            elif agg == "cv":
                row["real_cv"] = float(item.get("real_time", 0.0)) * 100.0
        elif run_type == "iteration":
            if "real_median" not in row:
                row["real_median"] = float(item.get("real_time", 0.0))
            if "cpu_median" not in row:
                row["cpu_median"] = float(item.get("cpu_time", 0.0))
            if "ips" not in row and "items_per_second" in item:
                row["ips"] = float(item.get("items_per_second", 0.0))

    return rows


def extract_scene_meta(data: dict) -> tuple[str | None, str | None]:
    context = data.get("context")
    if not isinstance(context, dict):
        return None, None
    bench_py = context.get("bench_py")
    if not isinstance(bench_py, dict):
        return None, None
    scene_key = bench_py.get("scene_key")
    scene_name = bench_py.get("scene_name")
    return (
        scene_key if isinstance(scene_key, str) else None,
        scene_name if isinstance(scene_name, str) else None,
    )


def find_baseline_file() -> Path | None:
    baseline = RESULTS_DIR / "baseline.json"
    return baseline if baseline.exists() else None


def load_baseline_rows() -> tuple[dict[str, dict[str, float | str]], Path | None]:
    baseline_file = find_baseline_file()
    if baseline_file is None:
        return {}, None
    try:
        data = json.loads(baseline_file.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}, None
    return parse_benchmark_rows(data), baseline_file


def compare_status(curr_median: float | None, base_median: float | None) -> tuple[str, str]:
    if curr_median is None or base_median is None or base_median <= 0.0:
        return "-", ""

    delta_pct = (curr_median - base_median) / base_median * 100.0
    delta_text = f"{delta_pct:+.1f}%"
    if delta_pct <= -2.5:
        return delta_text, paint("лучше", COLOR_OK)
    if delta_pct >= 2.5:
        return delta_text, paint("хуже", COLOR_BAD)
    return delta_text, paint("~", COLOR_HINT)

def estimate_complexity_label(alpha: float) -> str:
    if alpha < 0.25:
        return "O(1)"
    if alpha < 0.75:
        return "O(log N)"
    if alpha < 1.35:
        return "O(N)"
    if alpha < 1.75:
        return "O(N log N)"
    if alpha < 2.35:
        return "O(N^2)"
    if alpha < 2.85:
        return "O(N^3)"
    return "O(N^k)"


def linear_fit(xs: list[float], ys: list[float]) -> tuple[float, float, float]:
    """
    Возвращает (slope, intercept, r2) для y = slope*x + intercept.
    """
    n = len(xs)
    if n < 2:
        return 0.0, 0.0, 0.0

    mean_x = sum(xs) / n
    mean_y = sum(ys) / n

    ss_xx = sum((x - mean_x) ** 2 for x in xs)
    if ss_xx <= 0.0:
        return 0.0, mean_y, 0.0

    ss_xy = sum((x - mean_x) * (y - mean_y) for x, y in zip(xs, ys))
    slope = ss_xy / ss_xx
    intercept = mean_y - slope * mean_x

    ss_tot = sum((y - mean_y) ** 2 for y in ys)
    ss_res = sum((y - (slope * x + intercept)) ** 2 for x, y in zip(xs, ys))
    r2 = 1.0 - (ss_res / ss_tot) if ss_tot > 0.0 else 1.0
    return slope, intercept, r2


def print_complexity_estimate(rows: dict[str, dict[str, float | str]],
                              metadata: dict[str, dict[str, str]]) -> None:
    grouped: dict[str, list[tuple[int, float]]] = {}

    for run_name, metrics in rows.items():
        n_str = bench_arg(run_name)
        if not n_str.isdigit():
            continue
        n = int(n_str)

        time_val = metrics.get("real_median")
        if not isinstance(time_val, float):
            time_val = metrics.get("real_mean")
        if not isinstance(time_val, float):
            continue
        if n <= 0 or time_val <= 0.0:
            continue

        grouped.setdefault(short_bench_id(run_name), []).append((n, time_val))

    if not grouped:
        return

    results: list[tuple[str, str, float, float, int]] = []
    for bench_id, points in grouped.items():
        uniq: dict[int, float] = {}
        for n, t in points:
            uniq[n] = t
        sorted_points = sorted(uniq.items(), key=lambda p: p[0])
        if len(sorted_points) < 3:
            continue

        xs = [math.log(float(n)) for n, _ in sorted_points]
        ys = [math.log(float(t)) for _, t in sorted_points]
        alpha, _, r2 = linear_fit(xs, ys)
        ru_name = metadata.get(bench_id, {}).get("ru", bench_id)
        results.append((bench_id, ru_name, alpha, r2, len(sorted_points)))

    if not results:
        return

    table_rows: list[list[str]] = []
    for bench_id, ru_name, alpha, r2, count in sorted(results, key=lambda r: r[0]):
        table_rows.append([
            ru_name,
            f"{alpha:.2f}",
            estimate_complexity_label(alpha),
            f"{r2:.3f}",
            str(count),
        ])

    headers = ["Тест", "alpha", "Сложность", "R2", "Точек"]
    widths = [len(h) for h in headers]
    for row in table_rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))

    print()
    print(paint("Оценка сложности:", COLOR_TITLE))
    header_line = "  " + " | ".join(headers[i].ljust(widths[i]) for i in range(len(headers)))
    sep_line = "  " + "-+-".join("-" * widths[i] for i in range(len(headers)))
    print(header_line)
    print(sep_line)
    for row in table_rows:
        rendered = [row[i].ljust(widths[i]) for i in range(len(headers))]
        rendered[2] = paint_complexity_label(rendered[2])
        r2_value = float(row[3])
        if r2_value >= 0.95:
            rendered[3] = paint(rendered[3], COLOR_OK)
        elif r2_value >= 0.85:
            rendered[3] = paint(rendered[3], COLOR_WARN)
        else:
            rendered[3] = paint(rendered[3], COLOR_BAD)
        rendered[4] = paint_points_count(rendered[4])
        print("  " + " | ".join(rendered))


def print_results_table(data: dict, metadata: dict[str, dict[str, str]], scene_name: str | None = None) -> None:
    rows = parse_benchmark_rows(data)
    if not rows:
        return

    baseline_rows, baseline_file = load_baseline_rows()
    has_baseline = baseline_file is not None
    current_scene_key, _ = extract_scene_meta(data)
    baseline_scene_key = None
    baseline_scene_name = None
    if baseline_file is not None:
        try:
            baseline_data = json.loads(baseline_file.read_text(encoding="utf-8"))
            baseline_scene_key, baseline_scene_name = extract_scene_meta(baseline_data)
        except (OSError, json.JSONDecodeError):
            baseline_scene_key = None
            baseline_scene_name = None

    def sort_key(item: tuple[str, dict[str, float | str]]) -> tuple[str, int, str]:
        run_name = item[0]
        base = short_bench_id(run_name)
        arg = bench_arg(run_name)
        if arg.isdigit():
            return base, int(arg), ""
        return base, 10**9, arg

    ordered = sorted(rows.items(), key=sort_key)
    table_data: list[list[str]] = []
    for idx, (run_name, metrics) in enumerate(ordered, 1):
        base_id = short_bench_id(run_name)
        ru_name = metadata.get(base_id, {}).get("ru", base_id)
        curr_time_val = metrics.get("real_median")
        if not isinstance(curr_time_val, float):
            curr_time_val = metrics.get("real_mean")

        row = [
            str(idx),
            ru_name,
            bench_arg(run_name),
            fmt_cv(metrics.get("real_cv")),       # type: ignore[arg-type]
            fmt_ips(metrics.get("ips")),          # type: ignore[arg-type]
        ]
        if has_baseline:
            baseline_metrics = baseline_rows.get(run_name, {})
            base_time_val = baseline_metrics.get("real_median")
            if not isinstance(base_time_val, float):
                base_time_val = baseline_metrics.get("real_mean")

            curr_median_val = curr_time_val if isinstance(curr_time_val, float) else None
            base_median_val = baseline_metrics.get("real_median") if isinstance(baseline_metrics.get("real_median"), float) else None
            if base_median_val is None and isinstance(base_time_val, float):
                base_median_val = base_time_val
            delta_text, _ = compare_status(curr_median_val, base_median_val)
            row.extend([
                fmt_time_ns(curr_time_val),  # type: ignore[arg-type]
                fmt_time_ns(base_time_val),  # type: ignore[arg-type]
                delta_text,
            ])
        else:
            row.append(fmt_time_ns(curr_time_val))  # type: ignore[arg-type]
        table_data.append(row)

    headers = ["#", "Тест", "N", "cv(%)", "items/s"]
    if has_baseline:
        headers.extend(["time", "baseline", "d (%)"])
    else:
        headers.append("time")
    widths = [len(h) for h in headers]
    for row in table_data:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))

    print()
    if has_baseline:
        baseline_text = paint("baseline найден", COLOR_OK)
        if baseline_scene_name:
            baseline_scene_text = paint(f"Сцена: {baseline_scene_name}", COLOR_INDEX_LIGHT_BLUE)
            print(f"{baseline_text} | {baseline_scene_text}")
        else:
            print(baseline_text)
    else:
        print(paint("baseline не найден", COLOR_ERROR))
    if has_baseline and current_scene_key and baseline_scene_key and current_scene_key != baseline_scene_key:
        print(paint("Внимание: baseline снят на другой сцене, сравнение может быть некорректным", COLOR_WARN))
        print()
    print(paint("Итоговая таблица:", COLOR_TITLE))
    header_line = "  " + " | ".join(headers[i].ljust(widths[i]) for i in range(len(headers)))
    sep_line = "  " + "-+-".join("-" * widths[i] for i in range(len(headers)))
    print(header_line)
    print(sep_line)
    col = {name: idx for idx, name in enumerate(headers)}
    n_values = [
        int(row[col["N"]])
        for row in table_data
        if row[col["N"]].isdigit()
    ]
    min_n = min(n_values) if n_values else 0
    max_n = max(n_values) if n_values else 0
    for row in table_data:
        rendered = [row[i].ljust(widths[i]) for i in range(len(headers))]
        rendered[col["#"]] = paint(rendered[col["#"]], COLOR_INDEX_LIGHT_BLUE)
        rendered[col["N"]] = paint_n_gradient(rendered[col["N"]], min_n, max_n)
        rendered[col["cv(%)"]] = paint_numeric(rendered[col["cv(%)"]], cv_mode=True)
        rendered[col["time"]] = paint_time_cell(rendered[col["time"]])
        if has_baseline:
            rendered[col["baseline"]] = paint_time_cell(rendered[col["baseline"]])
            rendered[col["d (%)"]] = paint_delta(rendered[col["d (%)"]])
        print("  " + " | ".join(rendered))

    print_complexity_estimate(rows, metadata)


def load_bench_metadata() -> dict[str, dict[str, str]]:
    """
    Читает метаданные бенчмарков из комментариев формата:
    // @bench_meta {"id":"Fixture/Test","ru":"...","group":"Симуляция/Силы"}
    """
    meta: dict[str, dict[str, str]] = {}
    pattern = re.compile(r"@bench_meta\s+(\{.*\})")

    for cpp in BENCHMARKS_ROOT.rglob("*.cpp"):
        if "build" in cpp.parts:
            continue
        try:
            text = cpp.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            text = cpp.read_text(encoding="utf-8", errors="ignore")

        for line in text.splitlines():
            m = pattern.search(line)
            if not m:
                continue
            try:
                obj = json.loads(m.group(1))
            except json.JSONDecodeError:
                continue
            bench_id = obj.get("id")
            if not bench_id:
                continue
            meta[bench_id] = {
                "ru": str(obj.get("ru", bench_id)),
                "group": str(obj.get("group", "Прочее")),
            }

    return meta


def die(msg: str) -> None:
    print(f"{COLOR_ERROR}Ошибка: {msg}{COLOR_RESET}", file=sys.stderr)
    sys.exit(1)


def ensure_results_dir() -> None:
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)


def saved_results() -> list[Path]:
    if not RESULTS_DIR.exists():
        return []
    return sorted(RESULTS_DIR.glob("*.json"), reverse=True)


def format_timestamp(path: Path) -> str:
    return path.stem.replace("_", " ", 1).replace("-", ":", 2)


def list_available_filters() -> list[str]:
    if not BINARY.exists():
        die(f"Бинарник не найден: {BINARY}\nСобери проект перед запуском.")

    result = subprocess.run(
        [str(BINARY), "--benchmark_list_tests=true"], capture_output=True, text=True
    )
    names = [line.strip() for line in result.stdout.splitlines() if line.strip()]

    groups: dict[str, None] = {}
    for name in names:
        parts = name.split("/")
        group = "/".join(parts[:2]) if len(parts) >= 2 else parts[0]
        groups[group] = None
    return list(groups.keys())


def list_benchmark_cases(filter_regex: str | None = None) -> list[str]:
    if not BINARY.exists():
        die(f"Бинарник не найден: {BINARY}\nСобери проект перед запуском.")

    result = subprocess.run(
        [str(BINARY), "--benchmark_list_tests=true"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return []

    names = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if not filter_regex:
        return names

    try:
        rx = re.compile(filter_regex)
    except re.error:
        return names
    return [name for name in names if rx.search(name)]


def normalize_benchmark_case(name: str) -> str:
    for suffix in ("_mean", "_median", "_stddev", "_cv"):
        if name.endswith(suffix):
            return name[: -len(suffix)]
    return name


def is_benchmark_result_line(line: str) -> bool:
    s = line.strip()
    if not s:
        return False
    token = s.split()[0]
    if "/" not in token:
        return False
    banned_prefixes = (
        "Benchmark",
        "Run",
        "CPU",
        "L1",
        "L2",
        "L3",
        "***WARNING***",
        "202",
    )
    return not token.startswith(banned_prefixes)


def format_eta(seconds: float | None) -> str:
    if seconds is None or seconds < 0 or math.isinf(seconds) or math.isnan(seconds):
        return "--:--"
    s = int(round(seconds))
    m, s = divmod(s, 60)
    h, m = divmod(m, 60)
    if h > 0:
        return f"{h:02d}:{m:02d}:{s:02d}"
    return f"{m:02d}:{s:02d}"


def scene_label(scene_key: str | None) -> str:
    if scene_key is None:
        return "-"
    for key, label in SCENES:
        if key == scene_key:
            return label
    return scene_key


def print_progress(done: int, total: int, current_case: str = "", start_ts: float | None = None, tick: int = 0) -> None:
    if not sys.stdout.isatty() or total <= 0:
        return

    spinner_frames = ("⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏")
    spin = spinner_frames[tick % len(spinner_frames)]
    width = 24
    ratio = min(1.0, max(0.0, done / float(total)))
    fill = int(width * ratio)
    bar = "█" * fill + "░" * (width - fill)

    eta_seconds: float | None = None
    elapsed_seconds: float | None = None
    if start_ts is not None and done > 0:
        elapsed = time.monotonic() - start_ts
        elapsed_seconds = elapsed
        per_test = elapsed / float(done)
        eta_seconds = per_test * float(max(0, total - done))
    elif start_ts is not None:
        elapsed_seconds = time.monotonic() - start_ts

    case_text = current_case if current_case else "-"
    if len(case_text) > 64:
        case_text = case_text[:61] + "..."

    spin_col = paint(spin, COLOR_PROGRESS_SPINNER)
    bar_col = paint(bar, COLOR_PROGRESS_BAR)
    count_col = paint(f"[{done}/{total}]", COLOR_PROGRESS_COUNT)
    case_col = paint(case_text, COLOR_PROGRESS_CASE)
    eta_col = paint(format_eta(eta_seconds), COLOR_PROGRESS_TIME)
    elapsed_col = paint(format_eta(elapsed_seconds), COLOR_PROGRESS_TIME)
    text = f"{spin_col} [{bar_col}] {count_col} | {case_col} | Elapsed {elapsed_col} | ETA {eta_col}"
    # Clear the whole current line before redrawing.
    sys.stdout.write("\r\033[2K" + paint(text, COLOR_HINT))
    sys.stdout.flush()

def classify_filter_group(filter_name: str) -> str:
    if filter_name.startswith("RendererFixture"):
        return "Рендер"
    if filter_name.startswith("SimulationFixture"):
        return "Симуляция"
    return "Прочее"

def pretty_filter_name(filter_name: str, metadata: dict[str, dict[str, str]]) -> str:
    if filter_name in metadata:
        return metadata[filter_name].get("ru", filter_name)

    parts = filter_name.split("/")
    if len(parts) < 2:
        return filter_name

    fixture, test_name = parts[0], parts[1]
    if fixture.startswith("RendererFixture<Renderer3D>"):
        return f"3D: {test_name}"
    if fixture.startswith("RendererFixture<Renderer2D>"):
        return f"2D: {test_name}"
    if fixture.startswith("SimulationFixture"):
        return re.sub(r"([a-z])([A-Z])", r"\1 \2", test_name)
    return test_name

def classify_by_metadata(filter_name: str, metadata: dict[str, dict[str, str]]) -> tuple[str, str | None]:
    """
    Возвращает (главная_группа, подгруппа_или_None).
    group в metadata может быть:
      "Симуляция/Силы"
      "Рендер/2D"
      "Симуляция"
    """
    if filter_name in metadata:
        raw_group = metadata[filter_name].get("group", "Прочее")
        parts = [p.strip() for p in raw_group.split("/", 1)]
        main = parts[0] if parts and parts[0] else "Прочее"
        sub = parts[1] if len(parts) > 1 and parts[1] else None
        return main, sub
    return classify_filter_group(filter_name), None


def run_benchmark(
    filter_regex: str | None,
    repetitions: int = 3,
    min_time: str | None = None,
    scene_key: str | None = None,
) -> dict:
    if not BINARY.exists():
        die(f"Бинарник не найден: {BINARY}")

    ensure_results_dir()
    tmp_json = RESULTS_DIR / "_tmp_run.json"
    cmd = [
        str(BINARY),
        "--benchmark_format=console",
        f"--benchmark_out={tmp_json}",
        "--benchmark_out_format=json",
        f"--benchmark_repetitions={repetitions}",
    ]
    if min_time:
        cmd += [f"--benchmark_min_time={min_time}"]
    if filter_regex:
        cmd += [f"--benchmark_filter={filter_regex}"]

    total_cases = len(list_benchmark_cases(filter_regex))
    seen_cases: set[str] = set()
    current_case = ""
    start_ts = time.monotonic()
    tick = 0
    print_progress(0, total_cases, current_case, start_ts, tick)

    process_env = os.environ.copy()
    if scene_key:
        process_env["CHEM_BENCH_SCENE"] = scene_key

    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        env=process_env,
    )

    assert process.stdout is not None
    tail_lines: list[str] = []
    for raw_line in process.stdout:
        line = raw_line.rstrip("\n")
        if line.strip():
            tail_lines.append(line)
            if len(tail_lines) > 20:
                tail_lines.pop(0)
        if is_benchmark_result_line(line):
            token = line.strip().split()[0]
            case = normalize_benchmark_case(token)
            current_case = case
            if case not in seen_cases:
                seen_cases.add(case)
            tick += 1
            print_progress(len(seen_cases), total_cases, current_case, start_ts, tick)

    return_code = process.wait()
    final_total = total_cases if total_cases > 0 else len(seen_cases)
    print_progress(final_total, final_total, current_case, start_ts, tick + 1)
    if sys.stdout.isatty():
        sys.stdout.write("\n\n")
        sys.stdout.flush()

    if return_code != 0:
        if tail_lines:
            print("\n".join(tail_lines), file=sys.stderr)
        die(f"Бенчмарк завершился с кодом {return_code}")

    try:
        if not tmp_json.exists():
            die(f"JSON-файл результата не создан: {tmp_json}")
        data = json.loads(tmp_json.read_text(encoding="utf-8"))
        context = data.get("context")
        if not isinstance(context, dict):
            context = {}
            data["context"] = context
        context["bench_py"] = {
            "scene_key": scene_key or "crystal3d",
            "scene_name": scene_label(scene_key or "crystal3d"),
        }
        # Каноничный "последний запуск" всегда обновляем после успешного прогона.
        last_run = RESULTS_DIR / "last_run.json"
        last_run.write_text(json.dumps(data, indent=2), encoding="utf-8")
        try:
            tmp_json.unlink()
        except OSError:
            pass
        return data
    except json.JSONDecodeError as e:
        preview = tmp_json.read_text(encoding="utf-8", errors="replace")[:300] if tmp_json.exists() else ""
        die(f"Не удалось распарсить JSON: {e}\n{preview}")


def save_result(data: dict, filter_used: str | None) -> Path:
    ensure_results_dir()
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    if filter_used:
        # Windows-safe filename suffix.
        safe_filter = re.sub(r'[<>:"/\\|?*()\s]+', "_", filter_used)
        safe_filter = re.sub(r"_+", "_", safe_filter).strip("._")
        suffix = f"_{safe_filter}" if safe_filter else "_filtered"
    else:
        suffix = "_all"
    path = RESULTS_DIR / f"{timestamp}{suffix}.json"
    path.write_text(json.dumps(data, indent=2), encoding="utf-8")
    print(paint(f"Сохранено: {path}", COLOR_OK))
    return path


def maybe_save_baseline(data: dict) -> bool:
    if not sys.stdin.isatty():
        return False

    answer = input("\nСохранить как baseline? [y/N]: ").strip().lower()
    if answer != "y":
        return False

    ensure_results_dir()
    baseline_path = RESULTS_DIR / "baseline.json"
    baseline_path.write_text(json.dumps(data, indent=2), encoding="utf-8")
    print(paint(f"Baseline сохранен: {baseline_path}", COLOR_OK))
    return True


def interactive_menu() -> tuple[str | None, int, bool, str | None, str]:
    print(f"{paint('=== Chemical Simulator Benchmarks ===', COLOR_TITLE)}\n")

    filters = list_available_filters()
    metadata = load_bench_metadata()
    grouped: dict[str, list[tuple[str, str]]] = {
        "Рендер": [],
        "Симуляция": [],
        "Прочее": [],
    }
    subgrouped: dict[str, dict[str, list[tuple[str, str]]]] = {}
    for f in filters:
        main_group, sub_group = classify_by_metadata(f, metadata)
        if main_group == "Рендер":
            # Для UX держим рендер в одной группе без подкатегорий 2D/3D.
            sub_group = None
        if main_group not in grouped:
            grouped[main_group] = []
        if sub_group:
            if main_group not in subgrouped:
                subgrouped[main_group] = {}
            subgrouped[main_group].setdefault(sub_group, []).append((f, pretty_filter_name(f, metadata)))
        else:
            grouped[main_group].append((f, pretty_filter_name(f, metadata)))

    menu_filters: list[str] = []
    print(f"{paint('Доступные бенчмарки:', COLOR_TITLE)}\n")

    menu_index = 1
    ordered_main_groups = []
    for default_group in ("Рендер", "Симуляция", "Прочее"):
        if default_group in grouped:
            ordered_main_groups.append(default_group)
    for extra_group in grouped.keys():
        if extra_group not in ordered_main_groups:
            ordered_main_groups.append(extra_group)

    for group_name in ordered_main_groups:
        entries = grouped.get(group_name, [])
        sub_entries_map = subgrouped.get(group_name, {})
        if not entries:
            has_sub = any(sub_entries_map.values())
            if not has_sub:
                continue
        print(f"\n  {paint(f'--- {group_name} ---', COLOR_GROUP)}")

        # Сначала подгруппы из metadata, затем элементы без подгруппы.
        for sub_name in sub_entries_map.keys():
            print(f"  {paint_subgroup_label(sub_name)}")
            for raw, pretty in sub_entries_map[sub_name]:
                print(f"  {paint(f'{menu_index})', COLOR_INDEX)} {pretty}")
                menu_filters.append(raw)
                menu_index += 1

        for raw, pretty in entries:
            print(f"  {paint(f'{menu_index})', COLOR_INDEX)} {pretty}")
            menu_filters.append(raw)
            menu_index += 1

    print(f"\n  {paint('0)', COLOR_INDEX)} Все")

    print()
    choice = input("Выбери номер(а) через пробел (Enter = все): ").strip()

    selected: str | None = None
    if choice == "" or choice == "0":
        selected = None
    else:
        tokens = choice.split()
        if len(tokens) == 1:
            try:
                idx = int(tokens[0]) - 1
                if 0 <= idx < len(menu_filters):
                    selected = menu_filters[idx]
                else:
                    die("Неверный номер")
            except ValueError:
                selected = choice
        else:
            if not all(token.isdigit() for token in tokens):
                die("Для множественного выбора укажи только номера через пробел")

            selected_filters: list[str] = []
            for token in tokens:
                idx = int(token) - 1
                if 0 <= idx < len(menu_filters):
                    selected_filters.append(menu_filters[idx])
                else:
                    die("Неверный номер")

            selected_filters = list(dict.fromkeys(selected_filters))
            selected = "(" + "|".join(re.escape(item) for item in selected_filters) + ")"

    rep_input = input("Количество прогонов [3]: ").strip()
    repetitions = int(rep_input) if rep_input.isdigit() else 3

    min_time = input("Минимальное время прогона [пусто = по умолчанию, пример: 5s]: ").strip()
    if min_time == "":
        min_time = None

    print()
    print(paint("Выбор сцены (для SimulationFixture):", COLOR_TITLE))
    default_scene_id = "crystal3d"
    default_scene_name = scene_label(default_scene_id)
    print(f"  {paint('0)', COLOR_INDEX)} {default_scene_name} (по умолчанию)")

    scene_options = [(sid, name) for sid, name in SCENES if sid != default_scene_id]
    for i, (_, ru_name) in enumerate(scene_options, 1):
        print(f"  {paint(f'{i})', COLOR_INDEX)} {ru_name}")

    scene_choice = input("Сцена [0]: ").strip()
    selected_scene = default_scene_id
    if scene_choice and scene_choice != "0":
        if not scene_choice.isdigit():
            die("Неверный номер сцены")
        scene_index = int(scene_choice) - 1
        if not (0 <= scene_index < len(scene_options)):
            die("Неверный номер сцены")
        selected_scene = scene_options[scene_index][0]

    return selected, repetitions, False, min_time, selected_scene


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Запускалка бенчмарков для Chemical Simulator",
    )
    parser.add_argument("--filter", metavar="REGEX", help="Фильтр бенчмарков (regex)")
    parser.add_argument("--save", action="store_true", help="Сохранить результат в results/")
    parser.add_argument("--list", action="store_true", help="Показать сохранённые результаты")
    parser.add_argument(
        "--repetitions",
        metavar="N",
        type=int,
        default=3,
        help="Количество прогонов (default: 3)",
    )
    parser.add_argument(
        "--min-time",
        metavar="TIME",
        default=None,
        help="Минимальное время прогона benchmark (пример: 5s, 500ms)",
    )
    parser.add_argument(
        "--scene",
        metavar="SCENE",
        choices=[scene_id for scene_id, _ in SCENES],
        default="crystal3d",
        help="Сцена для SimulationFixture: crystal3d | crystal2d | random_gas2d",
    )
    parser.add_argument("--open", action="store_true", help="Открыть view.html в браузере")
    args = parser.parse_args()

    if args.list:
        results = saved_results()
        if not results:
            print("Нет сохранённых результатов.")
        else:
            print("\nСохранённые результаты:\n")
            for p in results:
                print(f"  {format_timestamp(p)}  —  {p}")
        return

    if args.open:
        if not VIEW_HTML.exists():
            die(f"view.html не найден: {VIEW_HTML}")
        webbrowser.open(VIEW_HTML.as_uri())
        return

    repetitions = args.repetitions
    min_time = args.min_time
    selected_scene = args.scene
    if args.filter or args.save:
        filter_regex = args.filter
        save_flag = args.save
    else:
        filter_regex, repetitions, save_flag, min_time, selected_scene = interactive_menu()

    print()
    print(paint(f"Выбрана сцена: {scene_label(selected_scene)}", COLOR_INDEX_LIGHT_BLUE))
    print()
    data = run_benchmark(filter_regex, repetitions, min_time, selected_scene)
    metadata = load_bench_metadata()
    print_results_table(data, metadata, scene_label(selected_scene))

    maybe_save_baseline(data)

    if save_flag:
        save_result(data, filter_regex)


if __name__ == "__main__":
    main()
