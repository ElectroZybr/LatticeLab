file(GLOB_RECURSE LATTICE_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/TestMain.cpp"
    "${CMAKE_SOURCE_DIR}/Lattice/Engine/*/tests/unit/*.cpp"
    "${CMAKE_SOURCE_DIR}/Lattice/Plugins/*/tests/unit/*.cpp"
)

add_executable(LatticeTests ${LATTICE_TEST_SOURCES})

target_link_libraries(LatticeTests PRIVATE
    Lattice
    LatticeClassicMD
)
add_test(NAME LatticeTests COMMAND LatticeTests)
