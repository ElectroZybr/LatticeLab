-- Plain particle contracts, storage and spatial index.
-- src/NeighborList.* is not ported yet and is not built.
target("ParticleDynamics")
    set_kind("headeronly")
    add_headerfiles("(ParticleDynamics/*.hpp)")
    add_includedirs(".", {public = true})
    add_deps("StdData", "Tasks", {public = true})
    add_packages("glm", {public = true})

target("ParticleDynamics.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("ParticleDynamics")
