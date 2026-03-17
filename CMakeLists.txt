cmake_minimum_required(VERSION 3.17)
project(Chemical-simulator)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

set(CMAKE_CXX_STANDARD 20)

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

# --- Настройка SFML ---
FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.1
)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_INSTALL_PKGCONFIG_FILES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sfml)

# --- Настройка ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.89
)
FetchContent_MakeAvailable(imgui)
include_directories(${imgui_SOURCE_DIR})

# --- Настройка ImGui-SFML ---
FetchContent_Declare(
    imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v2.6.1
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
    GIT_TAG        v0.6.5
)
FetchContent_MakeAvailable(ImGuiFileDialog)
add_library(ImGuiFileDialog_lib STATIC
    ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp
)
target_include_directories(ImGuiFileDialog_lib PUBLIC
    ${imguifiledialog_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
)

set(SOURCES
    main.cpp
    interface.h
    interface.cpp
    Engine/math/Vec2D.cpp
    Engine/math/Vec3D.cpp
    Engine/physics/Atom.cpp
    Engine/physics/Bond.cpp
    Engine/physics/BondTable.cpp
    Engine/physics/SpatialGrid.cpp
    Engine/Camera.cpp
    Engine/Renderer.cpp
    Engine/Simulation.cpp
    Engine/SimBox.cpp
    Engine/Tools.cpp
)

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
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${imguifiledialog_SOURCE_DIR} ${imgui_SOURCE_DIR} )
