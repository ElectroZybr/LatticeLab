#ifndef GRADIENT_SH
#define GRADIENT_SH

vec3 turboColor(float t) {
    float x = clamp(0.1 + t * 0.75, 0.1, 0.85);

    const vec3 c0 = vec3(0.135725, 0.091412, 0.106667);
    const vec3 c1 = vec3(4.597373, 2.185608, 12.592549);
    const vec3 c2 = vec3(-42.327686, 4.805216, -60.109686);
    const vec3 c3 = vec3(130.588706, -14.019451, 109.074510);
    const vec3 c4 = vec3(-150.566627, 4.210863, -88.506588);
    const vec3 c5 = vec3(58.137451, 2.774745, 26.818275);

    vec3 res = c5;
    res = res * x + c4;
    res = res * x + c3;
    res = res * x + c2;
    res = res * x + c1;
    res = res * x + c0;

    return res;
}

#endif