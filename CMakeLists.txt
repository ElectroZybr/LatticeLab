cmake_minimum_required(VERSION 3.17)
project(Chemical-simulator)

# CMake 4.x: allow legacy dependencies that declare very old minimum policies.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

if (WIN32)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries on Windows" FORCE)
    set(SFML_USE_STATIC_STD_LIBS ON CACHE BOOL "Use static std libs" FORCE)
else()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries on Unix" FORCE)
endif()

include(FetchContent)

unset(IMGUI_INCLUDE_DIR CACHE)
unset(IMGUI_SFML_CONFIG_DIR CACHE)
unset(imgui_SOURCE_DIR CACHE)
unset(imgui_BINARY_DIR CACHE)

find_package(OpenGL REQUIRED)

# --- Настройка SFML ---
FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_INSTALL_PKGCONFIG_FILES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sfml)

# --- Настройка ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
include_directories(${imgui_SOURCE_DIR})

# --- Настройка ImGui-SFML ---
FetchContent_Declare(
    imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v3.0
)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_SFML_IF_FETCHCONTENT ON CACHE BOOL "" FORCE)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)
set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

FetchContent_MakeAvailable(imgui-sfml)

# --- Настройка ImGuiFileDialog ---
FetchContent_Declare(
    ImGuiFileDialog
    GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
    GIT_TAG        v0.6.8
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
)
FetchContent_MakeAvailable(glad)

# --- Настройка GLM ---
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

file(GLOB_RECURSE SOURCES 
    "main.cpp"
    "Engine/*.cpp"
    "GUI/*.cpp"
    "Rendering/*.cpp"
)
list(FILTER SOURCES EXCLUDE REGEX "build/.*")

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

if (WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app_icon.rc")
    if(EXISTS ${APP_ICON_RESOURCE})
        target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${APP_ICON_RESOURCE})
    endif()
endif ()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
    sfml-graphics 
    sfml-window 
    sfml-system 
    sfml-network 
    sfml-audio
    ImGui-SFML::ImGui-SFML
    ImGuiFileDialog_lib
    glad
    OpenGL::GL
    glm::glm
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imguifiledialog_SOURCE_DIR}
)
