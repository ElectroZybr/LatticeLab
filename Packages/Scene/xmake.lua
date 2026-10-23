-- The rendering building blocks are a module without a branch of their own.
includes("../Graphics")

-- Composition: the scene branch of one window, assembled in code.
target("Scene")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Lattice", "Graphics.core")
