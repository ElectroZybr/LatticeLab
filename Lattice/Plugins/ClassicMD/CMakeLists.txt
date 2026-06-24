file(GLOB_RECURSE CLASSIC_MD_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE CLASSIC_MD_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

foreach(classic_md_file_list IN ITEMS CLASSIC_MD_SOURCES CLASSIC_MD_HEADERS)
    list(FILTER ${classic_md_file_list} EXCLUDE REGEX ".*/tests/.*")
endforeach()

add_library(LatticeClassicMD OBJECT
    ${CLASSIC_MD_SOURCES}
    ${CLASSIC_MD_HEADERS}
)

target_include_directories(LatticeClassicMD PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(LatticeClassicMD PUBLIC
    glm::glm
)
