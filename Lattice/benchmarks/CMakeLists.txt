file(GLOB_RECURSE BENCHMARK_SOURCES CONFIGURE_DEPENDS
    "physics/*.cpp"
    "rendering/*.cpp"
    "BenchmarkScenes.cpp"
    "bench_main.cpp"
)

add_executable(benchmarks ${BENCHMARK_SOURCES})
add_executable(bench
    BmRunner/Support.cpp
    BmRunner/Data.cpp
    BmRunner/Style.cpp
    BmRunner/Output.cpp
    BmRunner/Runner.cpp
)
target_compile_options(benchmarks PRIVATE
    $<$<AND:$<BOOL:${OPTIMIZE_FOR_NATIVE}>,$<NOT:$<CONFIG:Debug>>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-march=native>
)
set_target_properties(benchmarks PROPERTIES
    C_COMPILER_LAUNCHER ""
    CXX_COMPILER_LAUNCHER ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_target_properties(bench PROPERTIES
    C_COMPILER_LAUNCHER ""
    CXX_COMPILER_LAUNCHER ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(benchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_include_directories(bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(benchmarks PRIVATE
    benchmark::benchmark
    latticelab_lib
)

target_link_libraries(bench PRIVATE
    benchmark::benchmark
)
