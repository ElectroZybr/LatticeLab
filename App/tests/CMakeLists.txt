add_executable(AppTests
    TestMain.cpp
    unit/SceneCatalogTests.cpp
)

target_link_libraries(AppTests PRIVATE
    latticelab_lib
)

target_copy_webgpu_binaries(AppTests)

add_test(NAME app.module COMMAND AppTests)
