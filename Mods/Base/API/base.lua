---@diagnostic disable: undefined-global, lowercase-global

---@class ScriptAPI
---@field clear fun(self: ScriptAPI)
---@field set_world_title fun(self: ScriptAPI, title: string)
---@field set_box fun(self: ScriptAPI, x: number, y: number, z?: number)
---@field world_size fun(self: ScriptAPI): number, number, number
---@field load_molecules fun(self: ScriptAPI, path: string): integer, string[]
---@field begin_batch fun(self: ScriptAPI): ScriptBatch
---@field lj_min fun(self: ScriptAPI, speciesA: string, speciesB: string): number
---@field random_fill fun(self: ScriptAPI, region: table, composition: table, options: table|nil): integer
---@field lattice_fill fun(self: ScriptAPI, region: table, composition: table, options: table|nil): integer

---@type ScriptAPI
local scene_api = scene
scene = scene_api

atoms = dofile("Mods/Base/API/atoms.lua")
atom = atoms
molecule = dofile("Mods/Base/API/molecule.lua")

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

center = make_anchor("center")
fullworld = make_anchor("fullworld")

local primitive_registry = {}

local function read_vec3(value, label)
    assert(type(value) == "table", (label or "vec3") .. " must be a table")
    return {
        value.x or value[1] or 0,
        value.y or value[2] or 0,
        value.z or value[3] or 0,
    }
end

local function read_shrink(value, label)
    if type(value) == "number" then
        return { value, value, value }
    end
    return read_vec3(value, label)
end

local function resolve_region(scene_handle, region, context_name)
    local label = context_name or "primitive"
    assert(type(region) == "table", label .. " requires region table")

    local wx, wy, wz = scene_handle:world_size()

    if region.__region == "box" then
        local size = nil
        if type(region.size) == "table" and region.size.__anchor == "fullworld" then
            size = { wx, wy, wz }
            if region.size.__shrink ~= nil then
                local shrink = read_shrink(region.size.__shrink, label .. " region.size shrink")
                size = {
                    math.max(0.0, size[1] - shrink[1]),
                    math.max(0.0, size[2] - shrink[2]),
                    math.max(0.0, size[3] - shrink[3]),
                }
            end
        else
            size = read_vec3(assert(region.size, label .. " box region requires size"), label .. " region.size")
        end

        local min = { 0.0, 0.0, 0.0 }
        if region.center == center then
            min = {
                0.5 * (wx - size[1]),
                0.5 * (wy - size[2]),
                0.5 * (wz - size[3]),
            }
        elseif region.center ~= nil then
            local box_center = read_vec3(region.center, label .. " region.center")
            min = {
                box_center[1] - 0.5 * size[1],
                box_center[2] - 0.5 * size[2],
                box_center[3] - 0.5 * size[3],
            }
        elseif region.pos ~= nil then
            min = read_vec3(region.pos, label .. " region.pos")
        end

        return {
            kind = "box",
            min = min,
            size = size,
        }
    end

    if region.__region == "sphere" then
        local radius = assert(region.radius, label .. " sphere region requires radius")
        assert(type(radius) == "number" and radius > 0.0, label .. " sphere radius must be > 0")

        local sphere_center = { 0.0, 0.0, 0.0 }
        if region.center == center then
            sphere_center = { 0.5 * wx, 0.5 * wy, 0.5 * wz }
        elseif region.center ~= nil then
            sphere_center = read_vec3(region.center, label .. " region.center")
        end

        return {
            kind = "sphere",
            center = sphere_center,
            radius = radius,
        }
    end

    error(label .. " supports only box { ... } or sphere { ... } region")
end

local function register_primitive(kind, builder)
    assert(type(kind) == "string" and kind ~= "", "register_primitive requires non-empty kind")
    assert(type(builder) == "function", "register_primitive requires builder function")
    assert(primitive_registry[kind] == nil, "primitive '" .. kind .. "' is already registered")

    primitive_registry[kind] = builder
    _G[kind] = function(opts)
        opts = opts or {}
        opts.__kind = kind
        return opts
    end
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

register_primitive("load_molecules", function(scene_handle, opts)
    scene_handle:load_molecules(opts.path or "Mods/Base/Molecules")
end)

register_primitive("random_fill", function(scene_handle, opts)
    scene_handle:random_fill(
        resolve_region(scene_handle, assert(opts.region, "random_fill requires region"), "random_fill"),
        assert(opts.composition, "random_fill requires composition"),
        opts
    )
end)

register_primitive("lattice_fill", function(scene_handle, opts)
    scene_handle:lattice_fill(
        resolve_region(scene_handle, assert(opts.region, "lattice_fill requires region"), "lattice_fill"),
        assert(opts.composition, "lattice_fill requires composition"),
        opts
    )
end)

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
    local size = read_vec3(active_world.size or (active_world.box and active_world.box.size) or { 100, 100, 100 }, "world.size")

    scene:clear()
    scene:set_box(size[1], size[2], size[3])

    if active_world.name then
        scene:set_world_title(active_world.name)
    end

    for _, content_entry in ipairs(active_world.content or {}) do
        assert(type(content_entry) == "table", "world content entry must be a table")
        local kind = content_entry.__kind
        assert(type(kind) == "string" and kind ~= "", "world content entry requires __kind")
        local builder = primitive_registry[kind]
        assert(type(builder) == "function", "primitive '" .. kind .. "' is not registered")
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
    simulation = simulation,
    world = world,
    register_primitive = register_primitive,
}
