add_requires("glm")

-- Rendering building blocks over WGPU: Render, viewports, scene objects, cameras,
-- controllers. Ordinary classes; compose.cpp builds the render branch of a window.
target("Graphics.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("Graphics/*.cpp")
    add_includedirs(".", {public = true})
    add_packages("glm", {public = true})
    add_deps("WGPU.core", "Shell.core", {public = true})

-- Composition: the render branch of one window (Shell nests it).
target("Graphics")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Lattice", "Graphics.core", "ParticleDynamics")

target("Graphics.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Graphics.core")
    add_defines("GRAPHICS_SHADERS=\"" .. path.join(os.scriptdir(), "shaders"):gsub("\\", "/") .. "\"")
