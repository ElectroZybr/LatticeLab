-- Command queue of a thread that owns state (a simulation) and a pool that runs the
-- partitions of a CPU kernel: plain C++, no runtime.
target("Tasks")
    set_kind("headeronly")
    add_headerfiles("(Tasks/*.hpp)")
    add_includedirs(".", {public = true})

target("Tasks.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Tasks")
    if is_plat("linux") then
        add_syslinks("pthread")
    end
