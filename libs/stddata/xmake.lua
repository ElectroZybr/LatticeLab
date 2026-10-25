-- Header-only table structures. Independent of the Lattice runtime: only the
-- header-only Table/Exception contracts are used.
target("StdData")
    set_kind("headeronly")
    add_headerfiles("(stddata/*.hpp)")
    add_includedirs(".", {public = true})
    add_packages("lattice", {links = {}, syslinks = {}, public = true})

target("StdData.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("StdData")
