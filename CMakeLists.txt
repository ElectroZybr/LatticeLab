cmake_minimum_required(VERSION 3.17)
cmake_policy(SET CMP0069 NEW)
project(LatticeLab)

set(APP_NAME LatticeLab)
set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 1)
set(APP_VERSION_PATCH 01)
set(APP_VERSION_STRING "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")

option(OPTIMIZE_FOR_NATIVE "Enable native CPU optimizations outside Debug builds" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(ENABLE_IPO "Enable link-time optimization for non-Debug builds when supported" ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# CMake 4.x: allow legacy dependencies that declare very old minimum policies.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

if (WIN32)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries on Windows" FORCE)
else()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries on Unix" FORCE)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

include(dependencies)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "Engine/*.cpp"
    "GUI/*.cpp"
    "Rendering/*.cpp"
    "App/*.cpp"
)

set(GENERATED_ROOT "${CMAKE_BINARY_DIR}/gen_includes")
set(GENERATED_DIR "${GENERATED_ROOT}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")
configure_file(
    "${CMAKE_SOURCE_DIR}/App/AppVersion.h.in"
    "${GENERATED_DIR}/AppVersion.h"
    @ONLY
)

add_library(latticelab_lib STATIC ${CORE_SOURCES})

set(G_RES_HEADERS "")
include(embedded_resources)
add_dependencies(latticelab_lib embedded_resources)

include(shaders)
add_dependencies(latticelab_lib wgsl_shaders)

target_compile_options(latticelab_lib PRIVATE
    $<$<AND:$<BOOL:${OPTIMIZE_FOR_NATIVE}>,$<NOT:$<CONFIG:Debug>>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-march=native>
)

target_include_directories(latticelab_lib PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${GENERATED_ROOT}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imguifiledialog_SOURCE_DIR}
    ${webgpu_cpp_SOURCE_DIR}/wgpu-native
)

target_link_libraries(latticelab_lib PUBLIC
    glfw
    imgui ImGuiFileDialog_lib
    glm::glm
    webgpu
    zpp_bits
    libzstd_static
)

if (WIN32)
    add_executable(${APP_NAME} WIN32 main.cpp)
else()
    add_executable(${APP_NAME} main.cpp)
endif()

include(CheckIPOSupported)
if(ENABLE_IPO)
    check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
    if(lto_supported AND NOT MINGW)
        set_target_properties(latticelab_lib PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE
            INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE
        )
        set_target_properties(${APP_NAME} PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE
            INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE
        )
    elseif(MINGW)
        message(STATUS "IPO is disabled for MinGW")
    else()
        message(STATUS "IPO is not supported: ${lto_error}")
    endif()
endif()

target_link_libraries(${APP_NAME} PRIVATE latticelab_lib)
target_copy_webgpu_binaries(${APP_NAME})

include(windows_resources)

install(TARGETS ${APP_NAME} RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets" DESTINATION .)
if(EXISTS "${CMAKE_SOURCE_DIR}/demo/scenes")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/demo/scenes" DESTINATION demo)
endif()

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
 
    add_subdirectory(Benchmarks)
endif()
