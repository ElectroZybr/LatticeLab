--[[
Базовый Lua-генератор газа.
гибкая генерация газа из атомов или молекул через композитор

Ожидает composition = {
    { name = atom.Ar,      count = 1000 },
    { name = molecule.h2o, count = 500  },
}

Сам генератор не размещает частицы вручную:
- загружает базовые шаблоны молекул один раз;
- открывает batch в движке;
- для каждого элемента composition вызывает random_spawn(...).
]]

local gas = {}
local base_molecules_loaded = false
-- выполнение базового скрипта генераторов
local generator = dofile("Mods/Base/API/generator.lua")

function gas.build(scene, opts)
    local options = opts or {}
    local composition = generator.resolve_composition(opts, "gas.build")

    if not base_molecules_loaded then
        local loaded_count = scene:load_molecules("Mods/Base/Molecules")
        assert(loaded_count > 0, "no molecules were loaded from Mods/Base/Molecules")
        base_molecules_loaded = true
    end

    local batch = scene:begin_batch()
    local spawned = 0

    for _, entry in ipairs(composition) do
        for _ = 1, entry.count do
            if batch:random_spawn(entry.name, options) then
                spawned = spawned + 1
            end
        end
    end

    batch:finish()
    assert(spawned > 0, "generator failed to spawn any gas composition entries")
end

if type(register_content) == "function" then
    register_content("gas", gas.build)
end

return gas
