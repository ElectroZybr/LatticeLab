-- Runtime shell infrastructure: works with exports and the tree through views
-- passed by composition.
target("CLI.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("cli/*.cpp")
    add_includedirs(".", {public = true})
    add_deps("Lattice", {public = true})

target("CLI")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("CLI.core")

target("CLI.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("CLI.core")
