-- Input routing over runtime exports. ActionRouter is ordinary infrastructure code;
-- compose.cpp publishes one shared router for the application.
target("Actions.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("Actions/*.cpp")
    add_includedirs(".", {public = true})
    add_deps("Lattice", {public = true})

target("Actions")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Actions.core")

target("Actions.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Actions.core")
