file(GLOB_RECURSE LATTICE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE LATTICE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(Lattice STATIC
    ${LATTICE_SOURCES}
    ${LATTICE_HEADERS}
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
)

target_link_libraries(Lattice PUBLIC
    glm::glm
)
