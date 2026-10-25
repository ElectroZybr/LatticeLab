-- Package: publishes one shared GPU device (libs/wgpu) and resource inspection.
target("WGPU")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Lattice", "Wgpu")
