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

import argparse
import json
import re
import subprocess
import sys
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
COLOR_GROUP = "\033[1;36m"
COLOR_SUBGROUP = "\033[35m"
COLOR_INDEX = "\033[33m"
COLOR_INDEX_LIGHT_BLUE = "\033[96m"
COLOR_HINT = "\033[90m"
COLOR_ERROR = "\033[31m"
COLOR_OK = "\033[32m"
COLOR_WARN = "\033[33m"
COLOR_BAD = "\033[31m"


def paint(text: str, color: str) -> str:
    if not sys.stdout.isatty():
        return text
    return f"{color}{text}{COLOR_RESET}"


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
            cv = float(cell.strip())
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


def print_results_table(data: dict, metadata: dict[str, dict[str, str]]) -> None:
    rows = parse_benchmark_rows(data)
    if not rows:
        return

    baseline_rows, baseline_file = load_baseline_rows()
    has_baseline = baseline_file is not None

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
            fmt_num(curr_time_val),               # type: ignore[arg-type]
            fmt_num(metrics.get("real_cv")),      # type: ignore[arg-type]
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
                fmt_num(base_time_val),  # type: ignore[arg-type]
                delta_text,
            ])
        table_data.append(row)

    headers = ["#", "Тест", "N", "time(ns)", "cv(%)", "items/s"]
    if has_baseline:
        headers.extend(["baseline(ns)", "Δ (%)"])
    widths = [len(h) for h in headers]
    for row in table_data:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))

    print()
    if has_baseline:
        print(paint("baseline найден", COLOR_OK))
    else:
        print(paint("baseline не найден", COLOR_ERROR))
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
        if has_baseline:
            rendered[col["Δ (%)"]] = paint_delta(rendered[col["Δ (%)"]])
        print("  " + " | ".join(rendered))


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

    print(f"{paint('$ ' + ' '.join(cmd), COLOR_HINT)}\n", flush=True)

    result = subprocess.run(cmd, text=True)

    if result.returncode != 0:
        die(f"Бенчмарк завершился с кодом {result.returncode}")

    try:
        if not tmp_json.exists():
            die(f"JSON-файл результата не создан: {tmp_json}")
        return json.loads(tmp_json.read_text(encoding="utf-8"))
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


def interactive_menu() -> tuple[str | None, int, bool, str | None]:
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
            print(f"  {paint(f'[{sub_name}]', COLOR_SUBGROUP)}")
            for raw, pretty in sub_entries_map[sub_name]:
                print(f"      {paint(f'{menu_index})', COLOR_INDEX)} {pretty}")
                menu_filters.append(raw)
                menu_index += 1

        for raw, pretty in entries:
            print(f"      {paint(f'{menu_index})', COLOR_INDEX)} {pretty}")
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

    save = input("\nСохранить результат? [y/N]: ").strip().lower() == "y"
    return selected, repetitions, save, min_time


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
    if args.filter or args.save:
        filter_regex = args.filter
        save_flag = args.save
    else:
        filter_regex, repetitions, save_flag, min_time = interactive_menu()

    print()
    data = run_benchmark(filter_regex, repetitions, min_time)
    metadata = load_bench_metadata()
    print_results_table(data, metadata)

    baseline_saved = maybe_save_baseline(data)

    if save_flag:
        save_result(data, filter_regex)
    elif not baseline_saved:
        ensure_results_dir()
        tmp = RESULTS_DIR / "last_run.json"
        tmp.write_text(json.dumps(data, indent=2), encoding="utf-8")
        print(paint(f"Временный результат: {tmp}", COLOR_HINT))


if __name__ == "__main__":
    main()
