cmake_minimum_required(VERSION 3.28)
project(LatticeEngine LANGUAGES CXX)

file(GLOB_RECURSE LATTICE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(FILTER LATTICE_SOURCES EXCLUDE REGEX ".*/(CLI|benchmarks|build|_deps)/.*")

file(GLOB_RECURSE LATTICE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(FILTER LATTICE_HEADERS EXCLUDE REGEX ".*/(CLI|benchmarks|build|_deps)/.*")

add_library(Lattice STATIC
    ${LATTICE_SOURCES}
    ${LATTICE_HEADERS}
)

add_executable(Lattice-cli CLI/main.cpp)
target_link_libraries(Lattice-cli PRIVATE Lattice)
set_target_properties(Lattice-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_target_properties(Lattice-cli Lattice PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_compile_options(Lattice PRIVATE
    $<$<AND:$<BOOL:${OPTIMIZE_FOR_NATIVE}>,$<NOT:$<CONFIG:Debug>>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-march=native>
)

target_compile_definitions(Lattice PUBLIC
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
    $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
)

target_include_directories(Lattice PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include(FetchContent)

FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.5.0
)

FetchContent_MakeAvailable(CLI11)

target_link_libraries(Lattice-cli PRIVATE CLI11::CLI11)

if(BUILD_BENCHMARKS)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.5
        GIT_SHALLOW    ON
    )
    set(BENCHMARK_ENABLE_TESTING     OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_INSTALL_DOCS       OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL     OFF CACHE BOOL "" FORCE)
 
    FetchContent_MakeAvailable(googlebenchmark)
    add_subdirectory(benchmarks)
endif()
