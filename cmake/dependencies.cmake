include(FetchContent)

unset(IMGUI_INCLUDE_DIR CACHE)
unset(imgui_SOURCE_DIR CACHE)
unset(imgui_BINARY_DIR CACHE)

# --- Настройка GLFW ---
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    ON
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)
set(GLFW_EXPOSE_NATIVE_WAYLAND  OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# --- Настройка WebGPU ---
FetchContent_Declare(
    webgpu_distribution
    GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution.git
    GIT_TAG        wgpu-v24.0.0.2
    GIT_SHALLOW    ON
)
set(WEBGPU_BACKEND "WGPU" CACHE STRING "WebGPU backend" FORCE)
FetchContent_MakeAvailable(webgpu_distribution)

FetchContent_Declare(
    webgpu_cpp
    GIT_REPOSITORY https://github.com/eliemichel/WebGPU-Cpp.git
    GIT_TAG        wgpu-v24.0.3.1
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(webgpu_cpp)

# --- Настройка ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.3
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC
    webgpu
    glfw
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
set(GLFW_EXPOSE_NATIVE_WAYLAND 0)

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

# --- Настройка GLM ---
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(glm)

# --- Настройка zpp_bits ---
FetchContent_Declare(
    zpp_bits
    GIT_REPOSITORY https://github.com/eyalz800/zpp_bits.git
    GIT_TAG        v4.7
    GIT_SHALLOW    ON

)
FetchContent_Populate(zpp_bits)
add_library(zpp_bits INTERFACE)
target_include_directories(zpp_bits INTERFACE "${zpp_bits_SOURCE_DIR}")

# --- Настройка zstd ---
FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG        v1.5.6
    SOURCE_SUBDIR  build/cmake
)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build zstd shared library" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "Build zstd static library" FORCE)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Build zstd programs")
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "Build zstd tests")
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "Build zstd contrib")
set(ZSTD_BUILD_CONTRIB_TESTS OFF CACHE BOOL "Build zstd contrib tests")
set(ZSTD_BUILD_CONTRIB_EXAMPLES OFF CACHE BOOL "Build zstd contrib examples")
set(ZSTD_BUILD_CONTRIB_LIBS OFF CACHE BOOL "Build zstd contrib libs")
set(ZSTD_INSTALL OFF CACHE BOOL "Build zstd install")
FetchContent_MakeAvailable(zstd)
