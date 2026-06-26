file(GLOB_RECURSE LATTICE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE LATTICE_PUBLIC_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

foreach(lattice_file_list IN ITEMS LATTICE_SOURCES LATTICE_PUBLIC_HEADERS)
    list(FILTER ${lattice_file_list} EXCLUDE REGEX ".*/(CLI|benchmarks|build|tests|_deps|Plugins)/.*")
endforeach()

add_library(Lattice STATIC
    ${LATTICE_SOURCES}
    ${LATTICE_PUBLIC_HEADERS}
)

set_target_properties(Lattice PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

set(LATTICE_PLUGIN_TARGETS)
file(GLOB plugin_dirs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/Plugins" "${CMAKE_CURRENT_SOURCE_DIR}/Plugins/*")
foreach(plugin_dir IN LISTS plugin_dirs)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Plugins/${plugin_dir}/CMakeLists.txt")
        add_subdirectory("Plugins/${plugin_dir}")
    endif()
endforeach()

add_library(LatticeLab::Lattice ALIAS Lattice)

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

target_link_libraries(Lattice PUBLIC
    glm::glm
    lua::lua
    sol2
)

include(FetchContent)

FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.5.0
)

FetchContent_MakeAvailable(CLI11)

add_executable(Lattice-cli
    CLI/CliStyle.h
    CLI/main.cpp
    CLI/SystemInfo.cpp
    CLI/SystemInfo.h
)

set_target_properties(Lattice-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(Lattice-cli PRIVATE
    Lattice
    CLI11::CLI11
)

target_compile_definitions(Lattice-cli PRIVATE
    LL_CLI_ENGINE_VERSION="${APP_VERSION_STRING}"
    LL_CLI_BUILD_TYPE="$<CONFIG>"
    LL_CLI_COMPILER="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
)

add_subdirectory(tests)

if(BUILD_BENCHMARKS)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.5
        GIT_SHALLOW ON
    )

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googlebenchmark)
    add_subdirectory(benchmarks)
endif()
