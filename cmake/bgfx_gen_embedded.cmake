set(ALL_CONTENT "")
set(DATA_INITIALIZER "")

set(RENDERER_TYPES
    "Noop" "Agc" "Direct3D11" "Direct3D12" "Gnm" "Metal" "Nvn" "OpenGLES" "OpenGL" "Vulkan" "WebGPU"
)

set(S_glsl "OpenGL")
set(S_spv  "Vulkan")
set(S_mtl  "Metal")
set(S_dxbc "Direct3D11")
set(S_essl "OpenGLES")

foreach(R_TYPE ${RENDERER_TYPES})
    set(FOUND FALSE)
    set(CURRENT_SUFFIX "")
    
    foreach(S "glsl" "spv" "mtl" "dxbc" "essl")
        if("${S_${S}}" STREQUAL "${R_TYPE}")
            set(H_FILE "${SHADER_OUT}/${NAME}.${TYPE}.${S}.bin.h")
            if(EXISTS "${H_FILE}")
                set(FOUND TRUE)
                set(CURRENT_SUFFIX ${S})
            endif()
        endif()
    endforeach()

    if(FOUND)
        file(READ "${H_FILE}" CONTENT)
        
        string(REPLACE "static const uint8_t" "static constexpr uint8_t" SAFE_CONTENT "${CONTENT}")
        
        string(REPLACE "#pragma once" "" SAFE_CONTENT "${SAFE_CONTENT}")
        
        string(APPEND ALL_CONTENT "${SAFE_CONTENT}\n")
        
        set(ARR_NAME "${NAME}_${TYPE}_${CURRENT_SUFFIX}")
        string(APPEND DATA_INITIALIZER "        { bgfx::RendererType::${R_TYPE}, ${ARR_NAME}, sizeof(${ARR_NAME}) },\n")
    else()
        string(APPEND DATA_INITIALIZER "        { bgfx::RendererType::${R_TYPE}, nullptr, 0 },\n")
    endif()
endforeach()

file(WRITE "${OUT}"
"#pragma once
#include <bgfx/embedded_shader.h>
${ALL_CONTENT}
extern const bgfx::EmbeddedShader ${NAME}_${TYPE}_embedded;
inline constexpr bgfx::EmbeddedShader ${NAME}_${TYPE}_embedded = {
    \"${NAME}_${TYPE}\",
    {
${DATA_INITIALIZER}
    }
};
")