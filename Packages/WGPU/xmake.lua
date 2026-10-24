add_requires("wgpu-native")

-- RAII wrappers over wgpu-native: the only GPU backend, without an extra abstraction.
-- Implementation stays independent of runtime composition.
target("WGPU.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("WGPU/*.cpp")
    add_headerfiles("(WGPU/*.hpp)")
    add_includedirs(".", {public = true})
    add_packages("wgpu-native", {public = true})
    -- GpuSoA mirrors StdData::SoA tables; the copy is a command of the owner queue.
    add_deps("StdData", "Tasks", {public = true})
    -- Standalone Logger and header-only utilities; no runtime linkage.
    add_packages("lattice-logger", {public = true})

target("WGPU.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("WGPU.core")

-- Publishes one shared WGPU::Device and resource inspection.
target("WGPU")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Lattice", "WGPU.core")
