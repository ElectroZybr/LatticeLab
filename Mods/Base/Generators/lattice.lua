---@diagnostic disable: undefined-global

--[[
Базовый Lua-генератор кристаллической решетки.

Ожидает:
    lattice {
        structure = "bcc", -- или "hex"
        size = { 40, 40, 40 },
        pos = { 0, 0, 0 }, -- optional, или center
        spacing = 3.0, -- optional, по умолчанию берется max scene:lj_min(...) по composition
        margin = 0.0, -- optional
        composition = {
            { name = atom.Fe, fraction = 1.0 },
            { name = atom.C, fraction = 0.01 },
        }
    }

Сейчас реализована базовая поддержка:
- conventional BCC-cell
- hex-укладки

Fraction задает долю заполнения решетки.
Если видов несколько, внутри занятого site вид выбирается пропорционально fraction.
]]

local lattice = {}
local generator = dofile("Mods/Base/API/generator.lua")

local function read_vec3(value, label)
    assert(type(value) == "table", label .. " must be a table")
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

local function make_rng(seed)
    local state = math.floor(tonumber(seed) or os.time()) % 2147483647
    if state <= 0 then
        state = state + 2147483646
    end

    return function()
        state = (state * 48271) % 2147483647
        return state / 2147483647
    end
end

local function choose_species(composition, total_fraction, rng)
    local threshold = rng() * total_fraction
    local cumulative = 0.0

    for _, entry in ipairs(composition) do
        cumulative = cumulative + entry.fraction
        if threshold <= cumulative then
            return entry.name
        end
    end

    return composition[#composition].name
end

local function try_spawn_site(batch, composition, total_fraction, rng, position, opts, error_message)
    if rng() > total_fraction then
        return
    end

    assert(batch:spawn(choose_species(composition, total_fraction, rng), position, opts), error_message)
end

local function resolve_spacing(scene, opts, composition)
    if opts.spacing ~= nil then
        return opts.spacing
    end

    local dominant = composition[1]
    for i = 2, #composition do
        if composition[i].fraction > dominant.fraction then
            dominant = composition[i]
        end
    end

    local spacing = scene:lj_min(dominant.name, dominant.name)
    assert(spacing > 0.0, "lattice.build failed to resolve spacing from composition")
    return spacing
end

local function resolve_bounds(scene, opts)
    local size = nil
    local margin = opts.margin or 0.0
    local origin = { 0.0, 0.0, 0.0 }
    local wx, wy, wz = scene:world_size()

    if type(opts.size) == "table" and opts.size.__anchor == "fullworld" then
        size = { wx, wy, wz }
        if opts.size.__shrink ~= nil then
            local shrink = read_shrink(opts.size.__shrink, "lattice.build opts.size shrink")
            size = {
                math.max(0.0, size[1] - shrink[1]),
                math.max(0.0, size[2] - shrink[2]),
                math.max(0.0, size[3] - shrink[3]),
            }
        end
    else
        size = read_vec3(assert(opts.size, "lattice.build requires opts.size"), "lattice.build opts.size")
    end

    if opts.pos == center then
        origin = {
            0.5 * (wx - size[1]),
            0.5 * (wy - size[2]),
            0.5 * (wz - size[3]),
        }
    elseif opts.pos ~= nil then
        origin = read_vec3(opts.pos, "lattice.build opts.pos")
    end

    return {
        min = {
            origin[1] + margin,
            origin[2] + margin,
            origin[3] + margin,
        },
        size = {
            math.max(0.0, size[1] - 2.0 * margin),
            math.max(0.0, size[2] - 2.0 * margin),
            math.max(0.0, size[3] - 2.0 * margin),
        }
    }
end

local function spawn_bcc(batch, opts, composition, total_fraction, bounds)
    local spacing = opts.spacing
    local rng = make_rng(opts.seed)
    local nx = math.max(0, math.floor(bounds.size[1] / spacing))
    local ny = math.max(0, math.floor(bounds.size[2] / spacing))
    local nz = math.max(0, math.floor(bounds.size[3] / spacing))

    for z = 0, nz - 1 do
        for y = 0, ny - 1 do
            for x = 0, nx - 1 do
                local origin = {
                    bounds.min[1] + x * spacing,
                    bounds.min[2] + y * spacing,
                    bounds.min[3] + z * spacing,
                }
                local center = {
                    origin[1] + 0.5 * spacing,
                    origin[2] + 0.5 * spacing,
                    origin[3] + 0.5 * spacing,
                }

                try_spawn_site(batch, composition, total_fraction, rng, origin, opts,
                    "lattice.build failed to spawn BCC corner atom")
                try_spawn_site(batch, composition, total_fraction, rng, center, opts,
                    "lattice.build failed to spawn BCC center atom")
            end
        end
    end
end

local function spawn_hex(batch, opts, composition, total_fraction, bounds)
    local spacing = opts.spacing
    local rng = make_rng(opts.seed)

    local row_step = spacing * math.sqrt(3.0) * 0.5
    local layer_shift_y = spacing * math.sqrt(3.0) / 6.0
    local layer_step = spacing * math.sqrt(2.0 / 3.0)
    local nz = math.max(0, math.floor(bounds.size[3] / layer_step) + 1)

    for z = 0, nz - 1 do
        local is_b_layer = (z % 2) == 1
        local z_coord = bounds.min[3] + z * layer_step
        if z_coord > bounds.min[3] + bounds.size[3] then
            break
        end

        local ny = math.max(0, math.floor(bounds.size[2] / row_step) + 1)
        for y = 0, ny - 1 do
            local odd_row = (y % 2) == 1
            local x_offset = (odd_row and (0.5 * spacing) or 0.0) + (is_b_layer and (0.5 * spacing) or 0.0)
            local y_coord = bounds.min[2] + y * row_step + (is_b_layer and layer_shift_y or 0.0)
            if y_coord > bounds.min[2] + bounds.size[2] then
                break
            end

            local nx = math.max(0, math.floor((bounds.size[1] - x_offset) / spacing) + 1)
            for x = 0, nx - 1 do
                local x_coord = bounds.min[1] + x * spacing + x_offset
                if x_coord > bounds.min[1] + bounds.size[1] then
                    break
                end
                try_spawn_site(batch, composition, total_fraction, rng, { x_coord, y_coord, z_coord }, opts,
                    "lattice.build failed to spawn hex lattice particle")
            end
        end
    end
end

function lattice.build(scene, opts)
    local structure = assert(opts.structure, "lattice.build requires opts.structure")
    local composition, total_fraction = generator.resolve_fraction_composition(opts, "lattice.build")
    opts.spacing = resolve_spacing(scene, opts, composition)
    local bounds = resolve_bounds(scene, opts)
    local batch = scene:begin_batch()

    if structure == "bcc" then
        spawn_bcc(batch, opts, composition, total_fraction, bounds)
    elseif structure == "hex" then
        spawn_hex(batch, opts, composition, total_fraction, bounds)
    else
        error("unsupported lattice structure '" .. tostring(structure) .. "'")
    end

    batch:finish()
end

if type(register_content) == "function" then
    register_content("lattice", lattice.build)
end

return lattice
