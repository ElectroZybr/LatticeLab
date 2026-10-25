-- Plain integrators: halves of a step that composition places around the forces.
-- src/KDK.cpp and src/StepOps.hpp are not ported yet and are not built.
target("Integrators")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("integrators/Verlet.cpp")
    add_headerfiles("(integrators/*.hpp)")
    add_includedirs(".", {public = true})
    add_deps("ParticleDynamics", {public = true})

target("Integrators.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Integrators")
