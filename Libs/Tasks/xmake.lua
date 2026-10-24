-- Command queue of a thread that owns state (a simulation): plain C++, no runtime.
target("Tasks")
    set_kind("headeronly")
    add_headerfiles("(Tasks/*.hpp)")
    add_includedirs(".", {public = true})

target("Tasks.tests")
    set_kind("binary")
    set_targetdir("$(builddir)/tests")
    add_files("tests/*.cpp")
    add_deps("Tasks")
