> Язык: RU
> Статус EN: создан каркас
# Скрипты И Генераторы

Этот документ фиксирует целевую архитектуру Lua-сцен и генераторов.

## Принцип

- движок дает один базовый API
- bindings только открывают этот API для Lua
- генераторы сцен живут в Lua
- C++ не дублирует Lua-генераторы

Ключевая идея:

> scene DSL собирается в Lua, а движок предоставляет стабильные примитивы

## Слои

```text
Engine runtime
↓
Public Engine API
↓
Lua bindings
↓
Core DSL
↓
Lua generators / Mod DSL
↓
User scene script
```

### Engine

Engine отвечает за:

- физику
- состояние мира
- шаблоны молекул
- шаг симуляции
- быстрые операции размещения и спавна

Engine не должен знать:

- про Lua
- про mod DSL
- про готовые сценовые рецепты

### Simulation

`Simulation` координирует runtime:

- миры
- активный мир
- lifecycle симуляции
- update

`Simulation` не должен быть местом для DSL-логики и Lua-friendly sugar.

### Engine-facing API

Это стабильный внешний контракт поверх runtime.

Он нужен для:

- bindings
- app-layer
- тестов высокого уровня

Если такой слой появляется, он не должен быть копией `Simulation`.

### Lua bindings

Bindings:

- разбирают Lua-таблицы
- собирают C++-структуры
- вызывают engine-facing API

Bindings не содержат логики генераторов.

## Core DSL И Mod DSL

### Core DSL

Core DSL это минимальный стабильный язык сцены.

Примеры core-сущностей:

- `simulation`
- `world`
- `box`
- `settings`
- `content`
- `load_scene`
- `load_structure`
- `gas`
- `lattice`
- `atom`
- `molecule`
- `region`
- `placement`

Core DSL документируется как часть движка.

### Mod DSL

Mod DSL это sugar поверх core DSL, написанный на Lua.

Примеры:

- `solvent`
- `ions`
- `air`
- `protein_water`
- `steel_alloy`

Правило:

- движок не знает про mod DSL
- bindings не знают про mod DSL
- mod DSL раскрывается в core DSL или в базовые Lua API вызовы

Пример:

```lua
solvent {
    model = "water",
    density = 1.0
}
```

Это не часть движка. Это Lua-генератор, который может развернуться в:

```lua
gas {
    composition = {
        molecule { name = "H2O", count = 12345 }
    },
    placement = {
        mode = "random",
        collision = "reject"
    }
}
```

## Что Оставлять В C++

В C++ должны оставаться:

- парсинг форматов (`PDB`, `latbin` и т.д.)
- загрузка и хранение шаблонов
- быстрый spawn
- collision / fit checks
- spatial search
- rotation / sampling
- физика и update

В Lua должны жить:

- рецепты сцен
- композиция генераторов
- пресеты
- mod-level sugar

## Типичный Генератор

Базовый формат генератора в Lua:

```lua
local gas = {}

function gas.build(world, opts)
    local batch = world:begin_batch()

    -- batch:spawn_atom(...)
    -- batch:spawn_molecule(...)
    -- batch:random_position(...)
    -- batch:lattice(...)

    batch:finish()
end

return gas
```

Идея:

- генератор это обычный Lua-модуль
- генератор принимает `world` и `opts`
- генератор работает через batch-контекст
- batch создается через `world:begin_batch()`
- batch завершается через `batch:finish()`

Генератор не должен выполняться при `require`.

Он должен экспортировать функции сборки сцены.

Пример:

```lua
local gas = {}

function gas.build(world, opts)
    local batch = world:begin_batch()

    batch:random_gas {
        region = opts.region,
        temperature = opts.temperature,
        seed = opts.seed,
        composition = opts.composition
    }

    batch:finish()
end

return gas
```

## Критерий Для Новых Конструкций

Конструкция относится к core DSL, если:

- она базовая
- нужна многим сценам
- не привязана к одному модy
- не скрывает слишком много предметной логики

Конструкция относится к mod DSL, если:

- это sugar
- это предметная абстракция
- ее можно собрать из core-примитивов

## Примеры Сцен

### 1. Один Мир С Газовой Смесью

```lua
simulation {
    world {
        id = "gas_mix",

        box = {
            size = { 100, 100, 100 }
        },

        settings = {
            dt = 0.001,
            integrator = "verlet"
        },

        content = {
            gas {
                temperature = 300,
                seed = 12345,

                composition = {
                    atom     { element = "Ar",  count = 1000 },
                    molecule { name = "H2O",    count = 500  },
                    molecule { name = "O2",     count = 200  }
                }
            }
        }
    }
}
```

### 2. Два Региона В Одном Мире

```lua
simulation {
    world {
        id = "reactor",

        box = {
            size = { 300, 300, 300 }
        },

        content = {
            load_scene {
                path = "assets/reactor_body.latbin"
            },

            gas {
                region = {
                    min = { 0, 0, 0 },
                    max = { 150, 300, 300 }
                },

                composition = {
                    molecule { name = "H2", count = 5000 }
                }
            },

            gas {
                region = {
                    min = { 150, 0, 0 },
                    max = { 300, 300, 300 }
                },

                composition = {
                    molecule { name = "O2", count = 2500 }
                }
            }
        }
    }
}
```

### 3. Несколько Миров В Одной Симуляции

```lua
simulation {
    world {
        id = "argon",
        name = "Argon gas",

        box = {
            size = { 200, 200, 200 }
        },

        settings = {
            dt = 0.001,
            integrator = "verlet"
        },

        content = {
            gas {
                temperature = 300,

                placement = {
                    mode = "random",
                    collision = "reject"
                },

                composition = {
                    atom { element = "Ar", count = 5000 }
                }
            }
        }
    }

    world {
        id = "steel",
        name = "Steel crystal",

        box = {
            size = { 150, 150, 150 }
        },

        settings = {
            dt = 0.0005,
            integrator = "verlet"
        },

        content = {
            lattice {
                structure = "bcc",
                cells = { 40, 40, 40 },

                composition = {
                    atom { element = "Fe", fraction = 0.99 },
                    atom { element = "C",  fraction = 0.01 }
                }
            }
        }
    }
}
```

### 4. Mod DSL Поверх Core DSL

```lua
simulation {
    world {
        id = "protein_water",

        box = {
            size = { 500, 500, 500 }
        },

        settings = {
            dt = 0.001,
            integrator = "verlet"
        },

        content = {
            load_structure {
                path = "assets/hemoglobin.pdb"
            },

            solvent {
                model = "water",
                density = 1.0
            },

            ions {
                concentration = "150mM",

                composition = {
                    ion { element = "Na+" },
                    ion { element = "Cl-" }
                }
            }
        }
    }
}
```

`solvent` и `ions` здесь не обязаны быть частью движка. Это допустимый mod DSL поверх core DSL.

## Итог

- движок стабилизирует примитивы
- bindings остаются тонкими
- core DSL остается небольшим
- mod DSL расширяет язык без роста C++ API
