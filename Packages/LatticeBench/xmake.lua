-- Benchmark runner infrastructure over Lattice BmRunner.
target("Bench.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("LatticeBench/*.cpp")
    add_includedirs(".", {public = true})
    add_deps("Lattice", {public = true})

target("Bench")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Bench.core")

target("Bench.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Bench.core")
