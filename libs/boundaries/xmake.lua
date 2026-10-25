-- Simulation boundaries: the model and its executors side by side.
--   Boundaries      model: space, geometry program and builder, materials and laws, layout
--   Boundaries.cpu  executors on the CPU over StdData::SoA
--   Boundaries.wgpu executors on the GPU over Wgpu::GpuSoA
-- Plain C++: no Lattice runtime, no domain (atoms) and no rendering.
target("Boundaries")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("boundaries/*.cpp")
    add_headerfiles("(boundaries/*.hpp)")
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
    add_files("boundariescpu/*.cpp")
    add_headerfiles("(boundariescpu/*.hpp)")
    add_deps("Boundaries", "StdData", {public = true})

-- Executors on the GPU over Wgpu::GpuSoA: the same program, materials and laws in WGSL.
target("Boundaries.wgpu")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    -- WGSL sources are embedded into the library (utils.bin2c → <file>.wgsl.h).
    add_rules("utils.bin2c", {extensions = {".wgsl"}})
    add_files("boundarieswgpu/*.cpp", "boundarieswgpu/*.wgsl")
    add_headerfiles("(boundarieswgpu/*.hpp)")
    add_deps("Boundaries", "Wgpu", {public = true})

-- The model, every backend and their equivalence on the same data (GPU skipped without an adapter).
target("Boundaries.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Boundaries.cpu", "Boundaries.wgpu", "Tasks")

-- Benchmarks of the hot paths (BmRunner, no runtime): xmake run Boundaries.bench [name]
target("Boundaries.bench")
    set_kind("binary")
    set_targetdir("$(builddir)/bench")
    add_files("bench/*.cpp")
    add_deps("Boundaries.cpu", "Boundaries.wgpu", "Tasks")
    add_packages("lbm-runner")
