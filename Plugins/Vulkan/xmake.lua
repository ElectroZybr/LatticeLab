-- add_requires("vulkan-headers")
-- add_requires("vulkan-loader")

-- target("Vulkan")
--     set_kind("shared")
--     add_rules("lattice.plugin_codegen")
--     set_targetdir(".")

--     add_files("src/**.cpp")

--     add_packages("vulkan-headers", "vulkan-loader")

--     add_includedirs("include", {public = true})
--     add_includedirs("..", {public = true})
--     add_includedirs("../GPU/include", {public = true})

--     add_deps("Lattice")