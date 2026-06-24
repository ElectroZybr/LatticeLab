add_executable(RenderTests
    unit/CameraTests.cpp
)

target_link_libraries(RenderTests PRIVATE
    LatticeLab::Rendering
)

target_copy_webgpu_binaries(RenderTests)

add_test(NAME rendering.module COMMAND RenderTests)
