---@diagnostic disable: undefined-global

local generator = {}

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

function generator.make_options(default_options, overrides)
    local options = {}
    for key, value in pairs(default_options or {}) do
        options[key] = value
    end

    if overrides then
        for key, value in pairs(overrides) do
            options[key] = value
        end
    end

    return options
end

function generator.resolve_composition(opts, context_name)
    local label = context_name or "generator"
    assert(opts.composition ~= nil, label .. " requires opts.composition")
    assert(type(opts.composition) == "table", label .. " requires opts.composition to be a table")

    local entries = {}
    for index, entry in ipairs(opts.composition) do
        assert(type(entry) == "table", label .. " composition entry #" .. index .. " must be a table")
        local name = assert(entry.name, label .. " composition entry #" .. index .. " requires name")
        local count = assert(entry.count, label .. " composition entry #" .. index .. " requires count")
        entries[#entries + 1] = {
            name = name,
            count = count,
        }
    end

    assert(#entries > 0, label .. " requires at least one composition entry")
    return entries
end

function generator.resolve_fraction_composition(opts, context_name)
    local label = context_name or "generator"
    assert(opts.composition ~= nil, label .. " requires opts.composition")
    assert(type(opts.composition) == "table", label .. " requires opts.composition to be a table")

    local entries = {}
    local total_fraction = 0.0
    for index, entry in ipairs(opts.composition) do
        assert(type(entry) == "table", label .. " composition entry #" .. index .. " must be a table")
        local name = entry.name or entry.element
        local fraction = entry.fraction or 1.0
        assert(name, label .. " composition entry #" .. index .. " requires name or element")
        assert(type(fraction) == "number" and fraction > 0.0, label .. " composition entry #" .. index .. " fraction must be > 0")
        total_fraction = total_fraction + fraction
        entries[#entries + 1] = {
            name = name,
            fraction = fraction,
        }
    end

    assert(#entries > 0, label .. " requires at least one composition entry")
    assert(total_fraction > 0.0, label .. " requires total composition fraction > 0")
    assert(total_fraction <= 1.0, label .. " requires total composition fraction <= 1")
    return entries, total_fraction
end

function generator.resolve_region(scene, region, context_name)
    local label = context_name or "generator"
    assert(type(region) == "table", label .. " requires region table")

    local wx, wy, wz = scene:world_size()

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
            min = {
                sphere_center[1] - radius,
                sphere_center[2] - radius,
                sphere_center[3] - radius,
            },
            size = {
                2.0 * radius,
                2.0 * radius,
                2.0 * radius,
            },
        }
    end

    error(label .. " supports only box { ... } or sphere { ... } region")
end

return generator
