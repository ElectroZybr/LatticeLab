target("StdData")
    set_kind("shared")
    set_targetdir(".")

    add_files("PluginInit.cpp")
    add_files("src/**.cpp")

    add_includedirs("..", {public = true})
    add_includedirs("include", {public = true})
    add_includedirs("src")

    add_includedirs("../StdIo/include")

    add_deps("Lattice")

target("StdData.tests")
    set_kind("shared")
    set_targetdir(".")

    add_files("tests/*.cpp")

    add_includedirs("..")
    add_includedirs("include")
    add_includedirs("../StdIo/include")

    add_deps("Lattice")