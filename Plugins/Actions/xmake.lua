-- сборка плагина
target("Actions")
    set_kind("shared")
    add_rules("lattice.plugin_codegen")
    set_targetdir(".")

    add_files("src/**.cpp")

    add_includedirs("include", {public = true})
    add_includedirs("..", {public = true})

    add_deps("Lattice")

-- сборка тестов
target("Actions.tests")
    set_kind("shared")
    set_targetdir(".")

    add_files("tests/*.cpp")
    add_files("src/ActionMap.cpp")

    add_includedirs("include")
    add_includedirs("..")

    add_deps("Lattice")
