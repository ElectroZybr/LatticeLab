file(GLOB ALL_HEADERS "${SHADER_OUT}/*.embedded.h")

set(INCLUDES "")
set(ENTRIES "")

foreach(H_FILE ${ALL_HEADERS})
    get_filename_component(FILE_NAME ${H_FILE} NAME)
    
    if("${FILE_NAME}" STREQUAL "shader_registry.h")
        continue()
    endif()

    string(REGEX REPLACE "\\.embedded\\.h$" "" BASE_NAME ${FILE_NAME})
    
    string(REPLACE "." "_" VAR_NAME ${BASE_NAME})
    set(FULL_VAR_NAME "${VAR_NAME}_embedded")

    string(APPEND INCLUDES "#include \"${FILE_NAME}\"\n")
    string(APPEND ENTRIES "    ${FULL_VAR_NAME},\n")
endforeach()

file(WRITE "${OUT}"
"#pragma once
#include <bgfx/embedded_shader.h>
${INCLUDES}
static constexpr bgfx::EmbeddedShader ${REGISTRY_NAME}[] = {
${ENTRIES}
    BGFX_EMBEDDED_SHADER_END()
};
")