-- сборка плагина
target("TestCycleB")
    set_kind("shared")
    set_targetdir(".")
    add_rules("lattice.plugin_codegen")
    add_includedirs("..", {public = true})
    add_deps("Lattice")