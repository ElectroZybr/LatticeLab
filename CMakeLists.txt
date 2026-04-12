cmake_minimum_required(VERSION 3.17)
cmake_policy(SET CMP0069 NEW)
project(LatticeLab)
set(APP_NAME LatticeLab)
set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 1)
set(APP_VERSION_PATCH 01)
set(APP_VERSION_STRING "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
option(OPTIMIZE_FOR_NATIVE "Enable -march=native optimization" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CMake 4.x: allow legacy dependencies that declare very old minimum policies.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
if(lto_supported AND NOT MINGW)
    message(STATUS "LTO (IPO) is supported and enabled")
elseif(lto_supported AND MINGW)
    message(STATUS "LTO (IPO) is disabled for MinGW due to linker/LTO instability")
else()
    message(WARNING "LTO is not supported: ${lto_error}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

if (WIN32)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries on Windows" FORCE)
    set(SFML_USE_STATIC_STD_LIBS ON CACHE BOOL "Use static std libs" FORCE)
else()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries on Unix" FORCE)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

include(FetchContent)

unset(IMGUI_INCLUDE_DIR CACHE)
unset(IMGUI_SFML_CONFIG_DIR CACHE)
unset(imgui_SOURCE_DIR CACHE)
unset(imgui_BINARY_DIR CACHE)

# --- Настройка SFML ---
FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
    GIT_SHALLOW    ON
)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_INSTALL_PKGCONFIG_FILES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sfml)

# --- Настройка ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/GUI/imgui_bgfx/imgui_impl_bgfx.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/GUI/imgui_bgfx
    ${bgfx_cmake_SOURCE_DIR}/bgfx/include
    ${bgfx_cmake_SOURCE_DIR}/bgfx/examples/common/imgui
    ${bgfx_cmake_SOURCE_DIR}/bx/include
)
target_link_libraries(imgui PUBLIC bgfx bx)

# --- Настройка ImGuiFileDialog ---
FetchContent_Declare(
    ImGuiFileDialog
    GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
    GIT_TAG        v0.6.8
    GIT_SHALLOW    ON
)
FetchContent_Populate(ImGuiFileDialog)
add_library(ImGuiFileDialog_lib STATIC
    ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp
)
target_include_directories(ImGuiFileDialog_lib PUBLIC
    ${imguifiledialog_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
)
target_link_libraries(ImGuiFileDialog_lib PUBLIC imgui)

# --- Настройка glad ---
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(glad)

# --- Настройка GLM ---
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(glm)

# --- Настройка bgfx ---
FetchContent_Declare(
    bgfx_cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG        v1.142.9197-527
    GIT_SHALLOW    ON
    )
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS    ON  CACHE BOOL "" FORCE)
set(BGFX_WITH_WAYLAND   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(bgfx_cmake)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "Engine/*.cpp"
    "GUI/*.cpp"
    "Rendering/*.cpp"
    "App/*.cpp"
)
list(FILTER CORE_SOURCES EXCLUDE REGEX "build/.*")

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

configure_file(
    "${CMAKE_SOURCE_DIR}/App/AppVersion.h.in"
    "${GENERATED_DIR}/AppVersion.h"
    @ONLY
)

add_library(latticelab_lib STATIC ${CORE_SOURCES})
if(lto_supported)
    set_target_properties(latticelab_lib PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# --- Компиляция шейдеров bgfx ---
set(SHADERC_BIN "$<TARGET_FILE:shaderc>")
set(BGFX_SHADER_INCLUDE "${bgfx_cmake_SOURCE_DIR}/bgfx/src")
set(BGFX_VARYING_DEF "${CMAKE_SOURCE_DIR}/assets/shaders/bgfx/varying.def.sc")
set(BGFX_SHADER_OUT "${CMAKE_SOURCE_DIR}/assets/shaders/bgfx")
if(WIN32)
    set(BGFX_SHADER_PLATFORM "windows")
    set(BGFX_SHADER_PROFILE_VERT "150")
    set(BGFX_SHADER_PROFILE_FRAG "150")
elseif(APPLE)
    set(BGFX_SHADER_PLATFORM "osx")
    set(BGFX_SHADER_PROFILE_VERT "150")
    set(BGFX_SHADER_PROFILE_FRAG "150")
else()
    set(BGFX_SHADER_PLATFORM "linux")
    set(BGFX_SHADER_PROFILE_VERT "150")
    set(BGFX_SHADER_PROFILE_FRAG "150")
endif()
function(bgfx_compile_shader SC_FILE TYPE OUT_FILES_VAR)
    get_filename_component(NAME ${SC_FILE} NAME_WE)
    set(OUT_FILE "${BGFX_SHADER_OUT}/${NAME}.${TYPE}.bin")

    if(${TYPE} STREQUAL "v")
        set(PROFILE ${BGFX_SHADER_PROFILE_VERT})
    else()
        set(PROFILE ${BGFX_SHADER_PROFILE_FRAG})
    endif()

    add_custom_command(
        OUTPUT ${OUT_FILE}
        COMMAND ${SHADERC_BIN}
            -f ${SC_FILE}
            -o ${OUT_FILE}
            --type ${TYPE}
            --platform ${BGFX_SHADER_PLATFORM}
            -p ${PROFILE}
            --varyingdef ${BGFX_VARYING_DEF}
            -i ${BGFX_SHADER_INCLUDE}
        DEPENDS ${SC_FILE} ${BGFX_VARYING_DEF}
        VERBATIM
    )
    set(${OUT_FILES_VAR} ${${OUT_FILES_VAR}} ${OUT_FILE} PARENT_SCOPE)
endfunction()
file(GLOB BGFX_VERT_SOURCES "${BGFX_SHADER_OUT}/*.vert.sc")
file(GLOB BGFX_FRAG_SOURCES "${BGFX_SHADER_OUT}/*.frag.sc")
set(BGFX_SHADER_BINS "")
foreach(SC_FILE ${BGFX_VERT_SOURCES})
    bgfx_compile_shader(${SC_FILE} v BGFX_SHADER_BINS)
endforeach()
foreach(SC_FILE ${BGFX_FRAG_SOURCES})
    bgfx_compile_shader(${SC_FILE} f BGFX_SHADER_BINS)
endforeach()
add_custom_target(bgfx_shaders ALL DEPENDS ${BGFX_SHADER_BINS})
add_dependencies(bgfx_shaders shaderc)
add_dependencies(latticelab_lib bgfx_shaders)

if(OPTIMIZE_FOR_NATIVE)
    target_compile_options(latticelab_lib PRIVATE -O3 -march=native)
    else()
    target_compile_options(latticelab_lib PRIVATE -O3)
endif()

target_include_directories(latticelab_lib PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${GENERATED_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imguifiledialog_SOURCE_DIR}
    ${bgfx_cmake_SOURCE_DIR}/bgfx/include
    ${bgfx_cmake_SOURCE_DIR}/bx/include
)

target_link_libraries(latticelab_lib PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
    imgui
    ImGuiFileDialog_lib
    glad
    glm::glm
    bgfx
    bx
)

if (WIN32)
    add_executable(${APP_NAME} WIN32 main.cpp)
else()
    add_executable(${APP_NAME} main.cpp)
endif()

if(lto_supported)
    set_target_properties(${APP_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(OPTIMIZE_FOR_NATIVE)
    target_compile_options(${APP_NAME} PRIVATE -O3 -march=native)
else()
    target_compile_options(${APP_NAME} PRIVATE -O3)
endif()

if (WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/app_icon.rc")
    set(APP_VERSION_RESOURCE "${GENERATED_DIR}/app_version.rc")
    configure_file(
        "${CMAKE_SOURCE_DIR}/assets/app_version.rc.in"
        "${APP_VERSION_RESOURCE}"
        @ONLY
    )
    if(EXISTS "${APP_ICON_RESOURCE}")
        source_group("Resource Files" FILES "${APP_ICON_RESOURCE}")        
        target_sources(${APP_NAME} PRIVATE "${APP_ICON_RESOURCE}")
    else()
        message(WARNING "Resource file not found: ${APP_ICON_RESOURCE}")
    endif()
    if(EXISTS "${APP_VERSION_RESOURCE}")
        source_group("Resource Files" FILES "${APP_VERSION_RESOURCE}")
        target_sources(${APP_NAME} PRIVATE "${APP_VERSION_RESOURCE}")
    endif()
endif()

target_link_libraries(${APP_NAME} PRIVATE latticelab_lib)

install(TARGETS ${APP_NAME}
    RUNTIME DESTINATION .
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets"
    DESTINATION .
)

if(EXISTS "${CMAKE_SOURCE_DIR}/demo/scenes")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/demo/scenes"
        DESTINATION demo
    )
endif()

option(BUILD_BENCHMARKS "Build benchmarks" OFF)
if(BUILD_BENCHMARKS)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.5
        GIT_SHALLOW    ON
    )
    set(BENCHMARK_ENABLE_TESTING     OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL     OFF CACHE BOOL "" FORCE)
 
    FetchContent_MakeAvailable(googlebenchmark)
 
    add_subdirectory(Benchmarks)
endif()
