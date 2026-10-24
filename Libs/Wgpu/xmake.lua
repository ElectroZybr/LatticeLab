add_requires("wgpu-native")

-- GPU compute library over wgpu-native: device, buffers, layouts, command lists and
-- GPU data structures (GpuSoA). No rendering (the renderer builds its passes and
-- pipelines on it), no runtime Lattice.
target("Wgpu")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("Wgpu/*.cpp")
    add_headerfiles("(Wgpu/*.hpp)")
    add_includedirs(".", {public = true})
    add_packages("wgpu-native", {public = true})
    -- GpuSoA mirrors StdData::SoA tables; the copy is a command of the owner queue.
    add_deps("StdData", "Tasks", {public = true})
    -- Standalone Logger and header-only utilities; no runtime linkage.
    add_packages("lattice-logger", {public = true})

target("Wgpu.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_rules("utils.bin2c", {extensions = {".wgsl"}})
    add_files("tests/*.cpp", "tests/shaders/*.wgsl")
    add_deps("Wgpu")

-- GPU against CPU on the same work (BmRunner): xmake run Wgpu.bench [name]
target("Wgpu.bench")
    set_kind("binary")
    set_targetdir("$(builddir)/bench")
    add_rules("utils.bin2c", {extensions = {".wgsl"}})
    add_files("bench/*.cpp", "bench/shaders/*.wgsl")
    add_deps("Wgpu")
    add_packages("lattice-bmrunner")
