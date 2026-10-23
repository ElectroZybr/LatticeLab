target("Serialization.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("Serialization/*.cpp")
    add_includedirs(".", {public = true})
    add_deps("Lattice", "StdIo.core")

target("Serialization")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Serialization.core")
    add_deps("StdIo", {inherit = false})

target("Serialization.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Lattice", "StdIo.core")
    add_deps("Serialization", {inherit = false})
    set_runargs(path.absolute("../..", os.scriptdir()))
