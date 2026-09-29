target("Lattice")
    set_kind("shared")
    set_targetdir(".")

    add_files("Kernel/**.cpp")
    add_files("Tools/**.cpp")

    add_includedirs("..", {public = true})

    add_packages("glm", "toml++", {public = true})

-- сборка тестов
target("Lattice.tests")
    set_kind("shared")
    set_targetdir(".")

    add_files("tests/*.cpp")
    
    for _, dir in ipairs(os.dirs("tests/TestPlugins/*")) do
        includes(dir)
    end

    add_deps("Lattice")

-- -- публичные инструменты Lattice
-- target("LatticeTools")
--     set_kind("static")
--     set_languages("c++23")
--     add_files("Tools/*.cpp")
--     add_headerfiles("Tools/(Tools/**.hpp)")
--     add_includedirs("Tools/", {public = true})

-- сборка бенчмарков
target("Lattice.bench")
    set_kind("shared")
    set_targetdir(".")

    add_files("bench/*.cpp")

    add_deps("Lattice")