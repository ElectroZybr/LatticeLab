cmake_minimum_required(VERSION 3.17)
project(Chemical-simulator)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

if (UNIX AND NOT APPLE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries on Linux" FORCE)
endif ()

# Настройка SFML
if (WIN32)
    set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/sfml/lib/cmake/SFML")
    set(SFML_STATIC_LIBRARIES ON)
endif ()

find_package(SFML 2.5 COMPONENTS graphics window system network audio REQUIRED)

# Добавление ImGui и ImGui-SFML
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/imgui")
add_subdirectory(Libraries/imgui)
add_subdirectory(Libraries/imgui-sfml)

# Исходные файлы
add_executable(${CMAKE_PROJECT_NAME}
        # custom files:
        main.cpp
        interface.h
        interface.cpp
        # engine:
        Engine/Consts.h
        Engine/math/Vec2D.h
        Engine/math/Vec2D.cpp
        Engine/math/Vec3D.h
        Engine/math/Vec3D.cpp
        Engine/physics/Atom.h
        Engine/physics/Atom.cpp
        Engine/physics/Bond.h
        Engine/physics/Bond.cpp
        Engine/physics/BondTable.h
        Engine/physics/BondTable.cpp
        Engine/physics/SpatialGrid.h
        Engine/physics/SpatialGrid.cpp
        Engine/Camera.h
        Engine/Camera.cpp
        Engine/Renderer.h
        Engine/Renderer.cpp
        Engine/Simulation.h
        Engine/Simulation.cpp
        Engine/SimBox.h
        Engine/SimBox.cpp
        Engine/Tools.h
        Engine/Tools.cpp
        )

if (WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app_icon.rc")
    target_sources(Chemical-simulator PRIVATE ${APP_ICON_RESOURCE})
endif ()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE sfml-system sfml-network sfml-graphics sfml-window sfml-audio)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE imgui)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ImGui-SFML::ImGui-SFML)
