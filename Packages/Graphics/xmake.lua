add_requires("glm")

-- Rendering building blocks over WGPU: viewports, scene objects, cameras, controllers,
-- Render. No branch of its own: the Scene package composes them.
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

target("Graphics.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Graphics.core")
    add_defines("GRAPHICS_SHADERS=\"" .. path.join(os.scriptdir(), "shaders"):gsub("\\", "/") .. "\"")
