file(GLOB_RECURSE BENCH_SOURCES "*.cpp")

add_executable(benchmarks ${BENCH_SOURCES})

target_compile_options(benchmarks PRIVATE -O3 -march=native)

target_link_libraries(benchmarks PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    chemical_simulator_lib
)