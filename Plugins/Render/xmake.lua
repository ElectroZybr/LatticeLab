-- add_requires("wgpu-native")

-- target("Render")
--     set_kind("shared")
--     add_rules("lattice.plugin_codegen")
--     set_targetdir(".")

--     add_files("src/**.cpp")

--     add_packages("wgpu-native")

--     add_includedirs("include", {public = true})
--     add_includedirs("..", {public = true})
    
--     add_includedirs("../WGPU/include")
--     add_includedirs("../Shell/include")

--     add_deps("Lattice")