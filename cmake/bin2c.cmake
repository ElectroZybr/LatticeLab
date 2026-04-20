file(READ ${INPUT_FILE} HEX_CONTENT HEX)

string(REPEAT "[0-9a-f][0-9a-f]" 12 REGEX_PATTERN)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " C_ARRAY ${HEX_CONTENT})

file(WRITE ${OUTPUT_FILE} "#include <cstdint>\n\n")
file(APPEND ${OUTPUT_FILE} "const uint8_t ${VAR_NAME}[] = {\n${C_ARRAY}\n};\n")