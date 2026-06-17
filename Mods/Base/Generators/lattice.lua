---@diagnostic disable: undefined-global

--[[
Базовый Lua-генератор кристаллической решетки.

Ожидает:
    lattice {
        structure = "bcc", -- или "hex"
        size = { 40, 40, 40 },
        pos = { 0, 0, 0 }, -- optional, или center
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

local function random(seed)
    local state = math.floor(tonumber(seed) or os.time()) % 2147483647
    if state <= 0 then
        state = state + 2147483646
    end

    return function()
        state = (state * 48271) % 2147483647
        return state / 2147483647
    end
end

local function choose_species(composition, total_fraction, random_value)
    local threshold = random_value() * total_fraction
    local cumulative = 0.0

    for _, entry in ipairs(composition) do
        cumulative = cumulative + entry.fraction
        if threshold <= cumulative then
            return entry.name
        end
    end

    return composition[#composition].name
end

local function try_spawn_site(batch, composition, total_fraction, random_value, position, opts, error_message)
    if random_value() > total_fraction then
        return
    end

    assert(batch:spawn(choose_species(composition, total_fraction, random_value), position, opts), error_message)
end

local function resolve_spacing(scene, opts, composition)
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

local function resolve_region(scene, opts)
    if opts.region ~= nil then
        return generator.resolve_region(scene, opts.region, "lattice.build")
    end

    return generator.resolve_region(scene, box {
        size = opts.size,
        center = opts.pos,
        pos = opts.pos,
    }, "lattice.build")
end

local function contains(region, position)
    if region.kind == "box" then
        return true
    end

    if region.kind == "sphere" then
        local dx = position[1] - region.center[1]
        local dy = position[2] - region.center[2]
        local dz = position[3] - region.center[3]
        return dx * dx + dy * dy + dz * dz <= region.radius * region.radius
    end

    return false
end

local function spawn_bcc(batch, opts, composition, total_fraction, region, spacing)
    local random_value = random(opts.seed)
    local nx = math.max(0, math.floor(region.size[1] / spacing))
    local ny = math.max(0, math.floor(region.size[2] / spacing))
    local nz = math.max(0, math.floor(region.size[3] / spacing))

    for z = 0, nz - 1 do
        for y = 0, ny - 1 do
            for x = 0, nx - 1 do
                local origin = {
                    region.min[1] + x * spacing,
                    region.min[2] + y * spacing,
                    region.min[3] + z * spacing,
                }
                local site_center = {
                    origin[1] + 0.5 * spacing,
                    origin[2] + 0.5 * spacing,
                    origin[3] + 0.5 * spacing,
                }

                if contains(region, origin) then
                    try_spawn_site(batch, composition, total_fraction, random_value, origin, opts,
                        "lattice.build failed to spawn BCC corner atom")
                end
                if contains(region, site_center) then
                    try_spawn_site(batch, composition, total_fraction, random_value, site_center, opts,
                        "lattice.build failed to spawn BCC center atom")
                end
            end
        end
    end
end

local function spawn_hex(batch, opts, composition, total_fraction, region, spacing)
    local random_value = random(opts.seed)

    local row_step = spacing * math.sqrt(3.0) * 0.5
    local layer_shift_y = spacing * math.sqrt(3.0) / 6.0
    local layer_step = spacing * math.sqrt(2.0 / 3.0)
    local nz = math.max(0, math.floor(region.size[3] / layer_step) + 1)

    for z = 0, nz - 1 do
        local is_b_layer = (z % 2) == 1
        local z_coord = region.min[3] + z * layer_step
        if z_coord > region.min[3] + region.size[3] then
            break
        end

        local ny = math.max(0, math.floor(region.size[2] / row_step) + 1)
        for y = 0, ny - 1 do
            local odd_row = (y % 2) == 1
            local x_offset = (odd_row and (0.5 * spacing) or 0.0) + (is_b_layer and (0.5 * spacing) or 0.0)
            local y_coord = region.min[2] + y * row_step + (is_b_layer and layer_shift_y or 0.0)
            if y_coord > region.min[2] + region.size[2] then
                break
            end

            local nx = math.max(0, math.floor((region.size[1] - x_offset) / spacing) + 1)
            for x = 0, nx - 1 do
                local x_coord = region.min[1] + x * spacing + x_offset
                if x_coord > region.min[1] + region.size[1] then
                    break
                end
                local position = { x_coord, y_coord, z_coord }
                if contains(region, position) then
                    try_spawn_site(batch, composition, total_fraction, random_value, position, opts,
                        "lattice.build failed to spawn hex lattice particle")
                end
            end
        end
    end
end

function lattice.build(scene, opts)
    local structure = assert(opts.structure, "lattice.build requires opts.structure")
    local composition, total_fraction = generator.resolve_fraction_composition(opts, "lattice.build")
    local spacing = resolve_spacing(scene, opts, composition)
    local region = resolve_region(scene, opts)
    local batch = scene:begin_batch()

    if structure == "bcc" then
        spawn_bcc(batch, opts, composition, total_fraction, region, spacing)
    elseif structure == "hex" then
        spawn_hex(batch, opts, composition, total_fraction, region, spacing)
    else
        error("unsupported lattice structure '" .. tostring(structure) .. "'")
    end

    batch:finish()
end

if type(register_content) == "function" then
    register_content("lattice", lattice.build)
end

return lattice
