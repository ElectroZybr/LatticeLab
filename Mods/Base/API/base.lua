---@diagnostic disable: undefined-global, lowercase-global

---@class ScriptBatch
---@field random_spawn fun(self: ScriptBatch, speciesName: string, options: table|nil): boolean
---@field finish fun(self: ScriptBatch)

---@class ScriptAPI
---@field clear fun(self: ScriptAPI)
---@field set_world_title fun(self: ScriptAPI, title: string)
---@field set_box fun(self: ScriptAPI, x: number, y: number, z?: number)
---@field world_size fun(self: ScriptAPI): number, number, number
---@field load_molecules fun(self: ScriptAPI, path: string): integer, string[]
---@field begin_batch fun(self: ScriptAPI): ScriptBatch
---@field lj_min fun(self: ScriptAPI, speciesA: string, speciesB: string): number

---@type ScriptAPI
local scene_api = scene
scene = scene_api

---@type atoms
local atoms_registry = dofile("Mods/Base/API/atoms.lua")
atoms = atoms_registry

atom = setmetatable({}, {
    __index = atoms_registry,
    __call = function(_, def)
        def.__kind = "atom"
        return def
    end,
})

---@type molecule
local molecule_registry = dofile("Mods/Base/API/molecule.lua")
molecule = molecule_registry

---@class SceneAnchor
---@field __anchor string

local function make_anchor(name)
    return setmetatable({
        __anchor = name,
    }, {
        __sub = function(anchor, value)
            return {
                __anchor = anchor.__anchor,
                __shrink = value,
            }
        end,
    })
end

---@type SceneAnchor
center = make_anchor("center")

---@type SceneAnchor
fullworld = make_anchor("fullworld")

local content_registry = {}

local function read_vec3(value, fallback)
    fallback = fallback or { 0, 0, 6 }
    if type(value) ~= "table" then
        return fallback[1], fallback[2], fallback[3]
    end

    return value.x or value[1] or fallback[1],
        value.y or value[2] or fallback[2],
        value.z or value[3] or fallback[3]
end

function world(def)
    def.__kind = "world"
    return def
end

function box(def)
    def.__region = "box"
    return def
end

function sphere(def)
    def.__region = "sphere"
    return def
end

function register_content(kind, builder)
    assert(type(kind) == "string" and kind ~= "", "register_content requires non-empty kind")
    assert(type(builder) == "function", "register_content requires builder function")
    assert(content_registry[kind] == nil, "content '" .. kind .. "' is already registered")

    content_registry[kind] = builder
    _G[kind] = function(opts)
        opts.__kind = kind
        return opts
    end
end

function simulation(def)
    local world_defs = {}
    for _, entry in ipairs(def) do
        if type(entry) == "table" and entry.__kind == "world" then
            world_defs[#world_defs + 1] = entry
        end
    end

    assert(#world_defs > 0, "simulation requires at least one world { ... }")
    assert(#world_defs == 1, "simulation currently supports exactly one world")

    local active_world = world_defs[1]
    local sx, sy, sz = read_vec3(active_world.size or (active_world.box and active_world.box.size), { 100, 100, 100 })

    scene:clear()
    scene:set_box(sx, sy, sz)

    if active_world.name then
        scene:set_world_title(active_world.name)
    end

    for _, content_entry in ipairs(active_world.content or {}) do
        assert(type(content_entry) == "table", "world content entry must be a table")
        local kind = content_entry.__kind
        assert(type(kind) == "string" and kind ~= "", "world content entry requires __kind")
        local builder = content_registry[kind]
        assert(type(builder) == "function", "content '" .. kind .. "' is not registered")
        builder(scene, content_entry)
    end
end

return {
    scene = scene,
    atom = atom,
    atoms = atoms,
    center = center,
    fullworld = fullworld,
    molecule = molecule,
    box = box,
    sphere = sphere,
    register_content = register_content,
    simulation = simulation,
    world = world,
}
