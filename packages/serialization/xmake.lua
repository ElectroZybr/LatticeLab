-- Runtime configuration serializer; the StdIo package places it in its branch.
target("Serialization.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("serialization/*.cpp")
    add_includedirs(".", {public = true})
    add_deps("Lattice", "StdIo.core", {public = true})

target("Serialization.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Lattice", "Serialization.core")
    add_deps("StdIo", {inherit = false})
    set_runargs(path.absolute("../..", os.scriptdir()))
