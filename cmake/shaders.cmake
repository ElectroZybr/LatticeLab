set(SHADERC_BIN "$<TARGET_FILE:shaderc>")
set(BIN2C_BIN "$<TARGET_FILE:bin2c>")
set(BGFX_SHADER_SRC "${CMAKE_SOURCE_DIR}/Rendering/shaders/bgfx")
set(BGFX_VARYING_DEF "${CMAKE_SOURCE_DIR}/Rendering/shaders/bgfx/varying.def.sc")
set(BGFX_SHADER_INCLUDE "${bgfx_cmake_SOURCE_DIR}/bgfx/src")
set(BGFX_SHADER_OUT "${GENERATED_DIR}/shaders")
file(MAKE_DIRECTORY "${BGFX_SHADER_OUT}")

# Платформа | Профиль Vertex | Профиль Fragment | Суффикс
set(BGFX_PLATFORMS
    "linux"   "410"    "410"    "glsl"
    "linux"   "spirv"  "spirv"  "spv"
    "osx"     "metal"  "metal"  "mtl"
    )
if(WIN32)
    list(APPEND BGFX_PLATFORMS 
        "windows" "s_5_0"  "s_5_0" "dxbc"
    )
endif()

function(bgfx_compile_shader_embedded SC_FILE TYPE OUT_HEADERS_VAR)
    get_filename_component(NAME ${SC_FILE} NAME_WE)

    set(H_FILES "")

    list(LENGTH BGFX_PLATFORMS LEN)
    math(EXPR LAST "${LEN} - 1")

    foreach(I RANGE 0 ${LAST} 4)
        list(GET BGFX_PLATFORMS ${I} PLATFORM)

        if(${TYPE} STREQUAL "v")
            math(EXPR PROFIL_IDX "${I}+1")
        else()
            math(EXPR PROFIL_IDX "${I}+2")
        endif()
        
        list(GET BGFX_PLATFORMS ${PROFIL_IDX} PROFILE)

        math(EXPR SUFFIX_IDX "${I}+3")
        list(GET BGFX_PLATFORMS ${SUFFIX_IDX} SUFFIX)

        set(BIN_FILE "${BGFX_SHADER_OUT}/${NAME}.${TYPE}.${SUFFIX}.bin")
        set(H_FILE   "${BGFX_SHADER_OUT}/${NAME}.${TYPE}.${SUFFIX}.bin.h")
        set(ARR_NAME "${NAME}_${TYPE}_${SUFFIX}") 

        add_custom_command(
            OUTPUT ${BIN_FILE}
            COMMAND ${SHADERC_BIN}
                -f ${SC_FILE} -o ${BIN_FILE}
                --type ${TYPE} --platform ${PLATFORM} -p ${PROFILE}
                --varyingdef ${BGFX_VARYING_DEF}
                -i ${BGFX_SHADER_INCLUDE}
                -i "${bgfx_cmake_SOURCE_DIR}/bx/include"
            DEPENDS ${SC_FILE} ${BGFX_VARYING_DEF} shaderc
            VERBATIM
        )

        add_custom_command(
            OUTPUT ${H_FILE}
            COMMAND ${BIN2C_BIN} -f ${BIN_FILE} -o ${H_FILE} -n ${ARR_NAME}
            DEPENDS ${BIN_FILE} bin2c
            VERBATIM
        )
        list(APPEND H_FILES ${H_FILE})
    endforeach()

    set(FINAL_H "${BGFX_SHADER_OUT}/${NAME}.${TYPE}.embedded.h")
    add_custom_command(
        OUTPUT ${FINAL_H}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${NAME}
            -DTYPE=${TYPE}
            -DOUT=${FINAL_H}
            -DSHADER_OUT=${BGFX_SHADER_OUT}
            -P ${CMAKE_SOURCE_DIR}/cmake/bgfx_gen_embedded.cmake
        DEPENDS ${H_FILES}
        VERBATIM
    )

    set(${OUT_HEADERS_VAR} ${${OUT_HEADERS_VAR}} ${FINAL_H} PARENT_SCOPE)
endfunction()

file(GLOB BGFX_VERT_SOURCES "${BGFX_SHADER_SRC}/*.vert.sc")
file(GLOB BGFX_FRAG_SOURCES "${BGFX_SHADER_SRC}/*.frag.sc")
set(BGFX_SHADER_HEADERS "")
foreach(SC_FILE ${BGFX_VERT_SOURCES})
    bgfx_compile_shader_embedded(${SC_FILE} v BGFX_SHADER_HEADERS)
endforeach()
foreach(SC_FILE ${BGFX_FRAG_SOURCES})
    bgfx_compile_shader_embedded(${SC_FILE} f BGFX_SHADER_HEADERS)
endforeach()
set(SHADER_REGISTRY_H "${BGFX_SHADER_OUT}/shader_registry.h")
add_custom_command(
    OUTPUT ${SHADER_REGISTRY_H}
    COMMAND ${CMAKE_COMMAND}
        -DSHADER_OUT=${BGFX_SHADER_OUT}
        -DREGISTRY_NAME=s_allShaders
        -DOUT=${SHADER_REGISTRY_H}
        -P ${CMAKE_SOURCE_DIR}/cmake/bgfx_gen_registry.cmake
    DEPENDS ${BGFX_SHADER_HEADERS}
    VERBATIM
)

add_custom_target(bgfx_registry DEPENDS ${SHADER_REGISTRY_H})
add_custom_target(bgfx_shaders ALL DEPENDS ${BGFX_SHADER_HEADERS})
add_dependencies(bgfx_shaders shaderc bin2c)
add_dependencies(bgfx_shaders bgfx_registry)
