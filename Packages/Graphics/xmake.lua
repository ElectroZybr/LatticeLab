add_requires("glm")

-- Rendering building blocks over WGPU: Render, viewports, scene objects, cameras,
-- controllers. Ordinary classes; compose.cpp builds the render branch of a window.
target("Graphics.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    -- WGSL sources are embedded into the library (utils.bin2c → <file>.wgsl.h).
    add_rules("utils.bin2c", {extensions = {".wgsl"}})
    add_files("Graphics/*.cpp", "Graphics/*.wgsl")
    add_includedirs(".", {public = true})
    add_packages("glm", {public = true})
    -- Boundaries.wgpu: the geometry machine, for drawing geometry programs.
    add_deps("Wgpu", "Shell.core", "Boundaries.wgpu", {public = true})

-- Composition: the render branch of one window (Shell nests it).
target("Graphics")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Lattice", "Graphics.core", "ParticleDynamics")

target("Graphics.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_rules("utils.bin2c", {extensions = {".wgsl"}})
    add_files("tests/*.cpp", "tests/shaders/*.wgsl")
    add_deps("Graphics.core")
    add_defines("GRAPHICS_SHADERS=\"" .. path.join(os.scriptdir(), "shaders"):gsub("\\", "/") .. "\"")

-- Frame costs of scene objects on the GPU (BmRunner, no runtime): xmake run Graphics.bench [name]
target("Graphics.bench")
    set_kind("binary")
    set_targetdir("$(builddir)/bench")
    add_files("bench/*.cpp")
    add_deps("Graphics.core")
    add_packages("lattice-bmrunner")
