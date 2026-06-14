> Язык: RU
> Статус EN: создан каркас
# Сборка

Эта статья описывает, как собирать проект, какие инструменты нужны и какие исполняемые файлы получаются на выходе.

## Требования

Проект собирается через `CMake` и требует `C++20`.

В репозитории используются `CMakePresets`.

Для сборки обычно нужны:

- `CMake` не ниже `3.23`
- `Ninja`
- компилятор с поддержкой `C++20`
- `Git`
- доступ в интернет при первой конфигурации, так как зависимости подтягиваются через `FetchContent`

На текущий момент `Python` для обычной сборки не требуется.

## Пресеты

Основные пресеты:

- `debug` — debug-сборка
- `release` — release-сборка
- `bench` — release-сборка с включенными benchmarks

Пресеты уже содержат базовые настройки проекта, включая:

- `Ninja` как generator
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON`
- `OPTIMIZE_FOR_NATIVE=ON`

## Базовая Сборка

Debug-сборка:

```bash
cmake --preset debug
cmake --build --preset debug
```

Release-сборка:

```bash
cmake --preset release
cmake --build --preset release
```

Сборка benchmarks:

```bash
cmake --preset bench
cmake --build --preset bench
```

## Результат Сборки

Основные исполняемые таргеты проекта:

- `LatticeLab` — desktop-приложение. Собирается обычной `debug` или `release` сборкой, бинарник лежит в корне репозитория: `./LatticeLab`.
- `Lattice-cli` — консольный запуск симуляции. Бинарник лежит в `./Lattice/Lattice-cli`.
- `bench` — benchmark runner. Бинарник лежит в `./Lattice/benchmarks/bench`.

## Связи

- [Входная Точка](../README.md)
- [Benchmarks](./Benchmarks.md)
- [Тесты](./Tests.md)
