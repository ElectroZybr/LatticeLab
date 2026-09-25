add_requires("stb")
add_requires("glfw")

target("Shell")
    set_kind("shared")
    add_rules("lattice.plugin_codegen")
    set_targetdir(".")

    add_files("src/**.cpp")
    if is_plat("macosx") then
        add_files("src/**.mm")
        add_frameworks("QuartzCore", "AppKit")
    end

    add_packages("stb")
    add_packages("glfw")
    if is_plat("linux") then
        add_syslinks("dl")
    end

    add_includedirs("include", {public = true})
    add_includedirs("..", {public = true})
    add_includedirs("src")

    add_includedirs("../Actions/include")
    add_includedirs("../Graphics/include")
    add_includedirs("../StdIo/include")
    add_includedirs("../GPU/include")

    add_deps("Lattice")

-- target("Shell.tests")
--     set_kind("shared")
--     set_targetdir(".")

--     add_files("tests/*.cpp")
--     add_files("../Actions/src/ActionMap.cpp")

--     add_includedirs("include")
--     add_includedirs("..")
--     add_includedirs("../Actions/include")
--     add_includedirs("../StdIo/include")

--     add_deps("Lattice")
