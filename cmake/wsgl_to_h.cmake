file(READ ${INPUT_FILE} CONTENT)
file(WRITE ${OUTPUT_FILE} "static const char* ${VAR_NAME} = R\"(\n${CONTENT}\n)\";\n")
message(STATUS "Generated WGSL header: ${OUTPUT_FILE}")