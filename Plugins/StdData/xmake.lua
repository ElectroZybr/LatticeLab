target("StdData")
    set_kind("shared")
    add_rules("lattice.plugin_codegen")
    set_targetdir(".")

    add_includedirs("..", {public = true})
    add_includedirs("include", {public = true})
    add_includedirs("src")

    add_includedirs("../StdIo/include")

    add_deps("Lattice")

