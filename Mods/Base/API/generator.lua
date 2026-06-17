local generator = {}

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

return generator
