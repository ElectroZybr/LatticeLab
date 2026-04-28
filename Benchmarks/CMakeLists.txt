file(GLOB_RECURSE BENCHMARK_SOURCES CONFIGURE_DEPENDS
    "physics/*.cpp"
    "rendering/*.cpp"
    "BenchmarkScenes.cpp"
)

add_executable(benchmarks ${BENCHMARK_SOURCES})

target_compile_options(benchmarks PRIVATE -O2)
set_target_properties(benchmarks PROPERTIES
    C_COMPILER_LAUNCHER ""
    CXX_COMPILER_LAUNCHER ""
)

target_include_directories(benchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(benchmarks PRIVATE
    benchmark::benchmark_main
    latticelab_lib
)
