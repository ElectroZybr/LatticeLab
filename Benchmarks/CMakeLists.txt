add_executable(benchmarks
    bench_main.cpp
    BenchmarkScenes.cpp
)

target_compile_options(benchmarks PRIVATE -O3 -march=native)

target_link_libraries(benchmarks PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    chemical_simulator_lib
)
