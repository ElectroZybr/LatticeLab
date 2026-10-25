-- Ordinary MD classes; usable without loading the package.
target("ClassicMD.core")
    set_kind("headeronly")
    add_headerfiles("(classicmd/*.hpp)")
    add_includedirs(".", {public = true})
    add_deps("StdData", "ParticleDynamics", "Integrators", "Tasks", "Boundaries.cpu", {public = true})
    -- Header-only Lattice::PipelineAPI; no runtime linkage.
    add_packages("lattice", {links = {}, syslinks = {}, public = true})

target("ClassicMD")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Lattice", "ClassicMD.core")

target("ClassicMD.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("ClassicMD.core")
