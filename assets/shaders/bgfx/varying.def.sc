vec2 v_texcoord0 : TEXCOORD0 = vec2(0.0, 0.0);
vec3 v_normal    : TEXCOORD1 = vec3(0.0, 0.0, 1.0);
vec3 v_fragPos   : TEXCOORD2 = vec3(0.0, 0.0, 0.0);
float v_atomCount : TEXCOORD3 = 0.0;
float v_maxCount  : TEXCOORD4 = 0.0;

vec3 a_position  : POSITION;
vec2 a_texcoord0 : TEXCOORD0;
vec3 a_texcoord1 : TEXCOORD1;
vec3 a_texcoord2 : TEXCOORD2;
