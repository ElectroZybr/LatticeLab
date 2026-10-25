set_project("LatticeLab")
local version = "0.1.4"
set_version(version)
add_defines('BUILD_VERSION="' .. version .. '"')
set_languages("c++23")

add_rules("mode.debug", "mode.release")

add_rules("plugin.compile_commands.autoupdate", {
    outputdir = "."
})

if is_mode("release") then
    add_cxxflags("-march=native")
end

option("lattice_source")
    set_default("../Lattice")
    set_showmenu(true)
    set_description("External Lattice source/package repository")
option_end()

option("lab_packages")
    set_default("")
    set_showmenu(true)
    set_description("Comma-separated package directories; empty builds all packages")
option_end()

local lattice_source = path.absolute(get_config("lattice_source") or "../Lattice")
add_repositories("lattice-local " .. lattice_source)
add_requires("lattice", {configs = {shared = true}})
add_requires("lattice-logger", "lbm-runner")
add_requires("glm 1.0.1", "toml++ 3.4.0", "yaml-cpp >=0.8.0")

-- Existing plugin targets inherit the installed external package through this target.
target("Lattice")
    set_kind("phony")
    add_packages("lattice", {public = true})
    on_load(function (target)
        local lattice = target:pkg("lattice")
        if lattice and not target:is_plat("windows") then
            target:add("rpathdirs", path.join(lattice:installdir(), "lib"), {public = true})
        end
    end)
target_end()

-- Ordinary libraries independent of the runtime; always available to packages.
for _, dir in ipairs(os.dirs("libs/*")) do
    if os.isfile(path.join(dir, "xmake.lua")) then
        includes(dir)
    end
end

local selected = get_config("lab_packages") or ""
for _, dir in ipairs(os.dirs("packages/*")) do
    if os.isfile(path.join(dir, "xmake.lua")) and
       (selected == "" or ("," .. selected .. ","):find("," .. path.filename(dir) .. ",", 1, true)) then
        includes(dir)
    end
end

target("LatticeLab")
    set_kind("binary")
    set_targetdir(".")
    
    add_files("main.cpp")

    add_deps("Lattice")