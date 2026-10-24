-- Simulation boundaries: the model and its executors side by side.
--   Boundaries      model: space, geometry program and builder, materials and laws, layout
--   Boundaries.cpu  executors on the CPU over StdData::SoA
-- Plain C++: no Lattice runtime, no domain (atoms) and no rendering.
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

target("Boundaries.cpu")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("BoundariesCpu/*.cpp")
    add_headerfiles("(BoundariesCpu/*.hpp)")
    add_deps("Boundaries", "StdData", {public = true})

target("Boundaries.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Boundaries.cpu")

-- Benchmarks of the hot paths (BmRunner, no runtime): xmake run Boundaries.bench [name]
target("Boundaries.bench")
    set_kind("binary")
    set_targetdir("$(builddir)/bench")
    add_files("bench/*.cpp")
    add_deps("Boundaries.cpu")
    add_packages("lattice-bmrunner")
