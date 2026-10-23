-- The serializer module has no branch of its own; StdIo composes it.
includes("../Serialization")

-- Ordinary implementation can be linked and used without loading a plugin.
target("StdIo.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("StdIo/*.cpp")
    add_includedirs(".", {public = true})
    add_packages("toml++", "yaml-cpp", {public = true})
    -- Only Value headers are used; ordinary IO does not link the runtime.
    add_packages("lattice", {links = {}, syslinks = {}})

target("StdIo")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Lattice", "StdIo.core", "Serialization.core")

target("StdIo.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_packages("lattice", {links = {}, syslinks = {}})
    add_deps("StdIo.core")
