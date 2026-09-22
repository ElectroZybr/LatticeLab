-- add_requires("glm")

-- target("Graphics")
--     set_kind("shared")
--     add_rules("lattice.plugin_codegen")
--     set_targetdir(".")

--     add_files("src/**.cpp")

--     add_packages("glm")

--     add_includedirs("include", {public = true})
--     add_includedirs("..", {public = true})

--     add_includedirs("../GPU/include")
--     add_includedirs("../Shell/include")

--     add_deps("Lattice")

-- target("Graphics.tests")
--     set_kind("shared")
--     set_targetdir(".")
--     add_files("tests/*.cpp", "src/Render.cpp")
--     add_includedirs("include", "..", "../Shell/include")
--     add_deps("Lattice")
