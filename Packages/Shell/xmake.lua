-- The router module has no branch of its own; Shell composes it.
includes("../Actions")

add_requires("stb")
add_requires("glfw")

-- Window, input devices, keybinds and the window loop: ordinary classes.
target("Shell.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("Shell/*.cpp")
    if is_plat("macosx") then
        add_files("Shell/*.mm")
        add_frameworks("QuartzCore", "AppKit")
    end
    add_packages("stb", "glfw", {public = true})
    if is_plat("linux") then
        add_syslinks("dl")
    end
    add_includedirs(".", {public = true})
    add_deps("StdIo.core", "Actions.core", "WGPU.core", {public = true})

target("Shell")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Shell.core")

target("Shell.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Shell.core")
