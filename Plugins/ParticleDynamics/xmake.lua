target("ParticleDynamics")
    set_kind("shared")
    add_rules("lattice.plugin_codegen")
    set_targetdir(".")

    -- Spatial algorithms are deferred in the minimal core.

    add_includedirs("..", {public = true})
    add_includedirs("include", {public = true})
    add_includedirs("src")

    add_includedirs("../StdData/include", {public = true})

    add_deps("Lattice")