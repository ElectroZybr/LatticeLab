target("CLI")
    set_kind("shared")
    add_rules("lattice.plugin_codegen")
    set_targetdir(".")

    add_files("src/**.cpp")

    add_includedirs("..", {public = true})
    add_includedirs("src")

    add_deps("Lattice")

target("CLI.tests")
    set_kind("shared")
    set_targetdir(".")

    add_files("tests/*.cpp")
    add_files("src/CommandLine.cpp")
    add_files("src/TreePath.cpp")

    add_includedirs("..", {public = true})
    add_deps("Lattice")
