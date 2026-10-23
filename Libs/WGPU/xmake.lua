add_requires("wgpu-native")

-- RAII wrappers over wgpu-native: the only GPU backend, without an extra abstraction.
-- Runtime-independent; the WGPU package (Packages/WGPU) publishes the shared device.
target("WGPU")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("WGPU/*.cpp")
    add_headerfiles("(WGPU/*.hpp)")
    add_includedirs(".", {public = true})
    add_packages("wgpu-native", {public = true})
    -- Logger and Exception only; no runtime structure.
    add_packages("lattice", {public = true})

target("WGPU.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("WGPU")
