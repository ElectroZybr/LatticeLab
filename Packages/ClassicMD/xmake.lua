-- Ordinary MD classes; usable without loading the plugin.
target("ClassicMD.core")
    set_kind("headeronly")
    add_headerfiles("(ClassicMD/*.hpp)")
    add_includedirs(".", {public = true})
    add_deps("StdData", "ParticleDynamics", "Integrators", {public = true})

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
