# Сборка и запуск

## Что нужно

- xmake, компилятор C++23 (GCC или Clang);
- ядро Lattice рядом: по умолчанию `../Lattice` (опция `lattice_source`). Ядро подключается как
  xmake-пакет — сторонние зависимости (glm, wgpu-native, yaml-cpp, toml++) xmake ставит сам.

## Сборка

```sh
xmake f -y                                  # конфигурация (по умолчанию — все пакеты)
xmake build -y                              # всё
xmake build -y Graphics                     # одна цель
```

- `--lab_packages=stdio,cli` — собрать только перечисленные пакеты (библиотеки собираются всегда).
- `--lattice_source=<path>` — другая копия ядра.
- Всегда передавай `-y`: без него xmake может молча ждать ответа на вопрос о конфигурации.
- Сборка всего проекта занимает около 30 секунд, инкрементальная — секунды. Дольше — значит
  что-то не так в окружении, а не в коде.

### После правки ядра

Ядро ставится как пакет и само не пересобирается. После правки исходников в `../Lattice`:

```sh
xmake require -f -y lattice              # или lbm-runner, lattice-logger, …
xmake build -y -r <цели, которые его используют>
```

Без `-r` объекты сохранят старую раскладку классов из заголовков ядра, и бинарник упадёт.

## Запуск

```sh
./LatticeLab
```

Что запускать, задаёт `lattice.toml`:

```toml
[Startup]
StdIo = "on"
WGPU = "on"
ClassicMD.Universe1 = "on"
Shell = ["on", "host"]      # сервис в host-потоке (окно)
CLI = ["on"]
```

Значения runtime по умолчанию — в `config/`. Изменения параметров, сделанные из CLI, сохраняются
в `user/runtime.yaml` и применяются при следующем запуске. Если удалённый или
переименованный параметр мешает запуску, убери его оттуда.

## Тесты

У каждого модуля одна цель тестов, без runtime:

```sh
xmake build -y Boundaries.tests && xmake run Boundaries.tests
```

GPU-проверки пропускаются, если нет адаптера.

## Бенчи

Бенчи — цели `<Name>.bench` (BmRunner). Быстрый прогон:

```sh
xmake run Graphics.bench SdfMesh
```

С историей результатов — `lbm` (ставится в ядре: `xmake install-lbm` в `../Lattice`):

```sh
lbm list                                   # наборы
lbm run Graphics SdfMesh --save -m "что изменилось"
lbm log                                    # сохранённые прогоны
lbm compare                                # последний против baseline или предыдущего
lbm regressions                            # что стало хуже
```

Результаты — git-репозиторий `lbm-results/` в корне проекта. Подробно —
[BmRunner](../../../Lattice/docs/ru/articles/BmRunner.md), [BmHistory](../../../Lattice/docs/ru/articles/BmHistory.md).
