local gas = {}

local default_options = {
    margin = 8.0,
    box = { 120, 120, 50 },
}

local function make_options(overrides)
    local options = {}
    for key, value in pairs(default_options) do
        options[key] = value
    end

    if overrides then
        for key, value in pairs(overrides) do
            options[key] = value
        end
    end

    return options
end

function gas.build(scene, opts)
    local options = make_options(opts)
    local name = assert(opts.name, "gas.build requires opts.name")
    local count = assert(opts.count, "gas.build requires opts.count")

    scene:clear()
    scene:set_box(options.box[1], options.box[2], options.box[3])

    local batch = scene:begin_batch()
    local spawned = 0

    for _ = 1, count do
        if batch:random_spawn(name, options) then
            spawned = spawned + 1
        end
    end

    batch:finish()
    assert(spawned > 0, "generator failed to spawn any species '" .. name .. "'")
end

return gas
