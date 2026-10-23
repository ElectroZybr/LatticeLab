target("Integrator")
    set_kind("shared")
    add_files("composition.cpp")
    set_targetdir(".")

    add_files("src/**.cpp")

    add_includedirs("..", {public = true})
    add_includedirs("src")

    add_includedirs("../ParticleDynamics/include", {public = true})

    add_deps("Lattice")