-- Simulation boundaries: space (box, periodicity), geometry (SDF program), walls.
-- Plain C++: no Lattice runtime, no GPU, no domain (atoms) and no rendering.
target("Boundaries")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("Boundaries/*.cpp")
    add_headerfiles("(Boundaries/*.hpp)")
    add_includedirs(".", {public = true})
    add_packages("glm", {public = true})
    -- Header-only Lattice::Exception; no runtime linkage.
    add_packages("lattice", {links = {}, syslinks = {}, public = true})

target("Boundaries.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Boundaries")
