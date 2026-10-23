-- Composition only: publishes one shared WGPU::Device. Classes live in Libs/WGPU.
target("WGPU.plugin")
    set_kind("shared")
    set_basename("WGPU")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Lattice", "WGPU")
