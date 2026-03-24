#!/usr/bin/env python3
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
import subprocess
import sys
import webbrowser
from datetime import datetime
from pathlib import Path

BINARY_NAME = "benchmarks" if sys.platform != "win32" else "benchmarks.exe"
BINARY = Path(__file__).parent.parent / BINARY_NAME
RESULTS_DIR = Path(__file__).parent / "results"
VIEW_HTML = Path(__file__).parent / "view.html"


def die(msg: str) -> None:
    print(f"\033[31mОшибка: {msg}\033[0m", file=sys.stderr)
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


def run_benchmark(filter_regex: str | None, repetitions: int = 3) -> dict:
    if not BINARY.exists():
        die(f"Бинарник не найден: {BINARY}")

    cmd = [
        str(BINARY),
        "--benchmark_format=json",
        f"--benchmark_repetitions={repetitions}",
    ]
    if filter_regex:
        cmd += [f"--benchmark_filter={filter_regex}"]

    print(f"\033[90m$ {' '.join(cmd)}\033[0m\n")

    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stderr)  # прогресс идёт в stderr

    if result.returncode != 0:
        die(f"Бенчмарк завершился с кодом {result.returncode}")

    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError as e:
        die(f"Не удалось распарсить JSON: {e}\n{result.stdout[:300]}")


def save_result(data: dict, filter_used: str | None) -> Path:
    ensure_results_dir()
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    suffix = f"_{filter_used.replace('/', '_')}" if filter_used else "_all"
    path = RESULTS_DIR / f"{timestamp}{suffix}.json"
    path.write_text(json.dumps(data, indent=2))
    print(f"\033[32mСохранено: {path}\033[0m")
    return path


def interactive_menu() -> tuple[str | None, int, bool]:
    print("\033[1m=== Chemical Simulator Benchmarks ===\033[0m\n")

    filters = list_available_filters()
    print("Доступные бенчмарки:\n")
    print("  0) Все")
    for i, f in enumerate(filters, 1):
        print(f"  {i}) {f}")

    print()
    choice = input("Выбери номер (Enter = все): ").strip()

    selected: str | None = None
    if choice == "" or choice == "0":
        selected = None
    else:
        try:
            idx = int(choice) - 1
            if 0 <= idx < len(filters):
                selected = filters[idx]
            else:
                die("Неверный номер")
        except ValueError:
            selected = choice  # ввели regex напрямую

    rep_input = input("Количество прогонов [3]: ").strip()
    repetitions = int(rep_input) if rep_input.isdigit() else 3

    save = input("\nСохранить результат? [y/N]: ").strip().lower() == "y"
    return selected, repetitions, save


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Запускалка бенчмарков для Chemical Simulator",
    )
    parser.add_argument("--filter", metavar="REGEX", help="Фильтр бенчмарков (regex)")
    parser.add_argument(
        "--save", action="store_true", help="Сохранить результат в results/"
    )
    parser.add_argument(
        "--list", action="store_true", help="Показать сохранённые результаты"
    )
    parser.add_argument(
        "--repetitions",
        metavar="N",
        type=int,
        default=3,
        help="Количество прогонов (default: 3)",
    )
    parser.add_argument(
        "--open", action="store_true", help="Открыть view.html в браузере"
    )
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
    if args.filter or args.save:
        filter_regex = args.filter
        save_flag = args.save
    else:
        filter_regex, repetitions, save_flag = interactive_menu()

    print()
    data = run_benchmark(filter_regex, repetitions)

    if save_flag:
        save_result(data, filter_regex)
    else:
        ensure_results_dir()
        tmp = RESULTS_DIR / "last_run.json"
        tmp.write_text(json.dumps(data, indent=2))
        print(f"\033[90mВременный результат: {tmp}\033[0m")

    if args.open:
        if not VIEW_HTML.exists():
            print(f"\033[33mview.html не найден рядом со скриптом\033[0m")
        else:
            webbrowser.open(VIEW_HTML.as_uri())


if __name__ == "__main__":
    main()
