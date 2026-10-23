add_requires("glm")

-- Rendering building blocks over WGPU: viewports, cameras, controllers, Render.
target("Graphics.core")
    set_kind("static")
    set_targetdir("$(builddir)/lib")
    if not is_plat("windows", "mingw") then
        add_cxflags("-fPIC")
    end
    add_files("Graphics/*.cpp")
    add_includedirs(".", {public = true})
    add_packages("glm", {public = true})
    add_deps("WGPU", "Shell.core", {public = true})

-- Composition: the render branch of one window.
target("Graphics")
    set_kind("shared")
    set_targetdir(".")
    add_files("compose.cpp")
    add_deps("Lattice", "Graphics.core")

target("Graphics.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Graphics.core")
    add_defines("GRAPHICS_SHADERS=\"" .. path.join(os.scriptdir(), "shaders"):gsub("\\", "/") .. "\"")
