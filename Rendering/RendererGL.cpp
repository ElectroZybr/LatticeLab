#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SFML/Window/Context.hpp>
#include <App/interaction/ToolsManager.h>
#include <Engine/metrics/Profiler.h>

#include "Engine/physics/Bond.h"
#include "RendererGL.h"

#ifdef __APPLE__
#include <iostream>
#endif

namespace {
void ensureGlFunction(bool available, const char* name) {
    if (!available) {
        throw std::runtime_error(std::string("OpenGL function is unavailable: ") + name);
    }
}

void initializeGlad(sf::RenderTarget& target) {
    if (!target.setActive(true)) {
        throw std::runtime_error("Failed to activate the SFML render target");
    }

    const auto loader = [](const char* name) -> void* {
        return reinterpret_cast<void*>(sf::Context::getFunction(name));
    };

    if (!gladLoadGLLoader(loader)) {
        throw std::runtime_error("gladLoadGLLoader failed to load OpenGL symbols");
    }

    ensureGlFunction(glad_glGenVertexArrays != nullptr, "glGenVertexArrays");
    ensureGlFunction(glad_glBindVertexArray != nullptr, "glBindVertexArray");
    ensureGlFunction(glad_glGenBuffers != nullptr, "glGenBuffers");
    ensureGlFunction(glad_glBindBuffer != nullptr, "glBindBuffer");
    ensureGlFunction(glad_glBufferData != nullptr, "glBufferData");
    ensureGlFunction(glad_glEnableVertexAttribArray != nullptr, "glEnableVertexAttribArray");
    ensureGlFunction(glad_glVertexAttribPointer != nullptr, "glVertexAttribPointer");
    ensureGlFunction(glad_glVertexAttribDivisor != nullptr, "glVertexAttribDivisor");
    ensureGlFunction(glad_glCreateShader != nullptr, "glCreateShader");
    ensureGlFunction(glad_glCreateProgram != nullptr, "glCreateProgram");
}
}

RendererGL::RendererGL(sf::RenderTarget& t, sf::View& gv, SimBox& simbox)
    : IRenderer(gv, simbox), target(t)
{
    initializeGlad(t);
    initQuadGL();
    initAtomGL();
    initBoxGL();
    initBondGL();
    initGridGL();
}

RendererGL::~RendererGL() {
    glDeleteVertexArrays(1, &atomVao);
    glDeleteVertexArrays(1, &atomVbo);
    glDeleteBuffers(1, &quadVbo);
    for (GLuint shader : atomShaders) {
        if (shader != 0) {
            glDeleteProgram(shader);
        }
    }

    glDeleteVertexArrays(1, &boxVao);
    glDeleteBuffers(1, &boxVbo);
    glDeleteProgram(boxShader);

    glDeleteVertexArrays(1, &bondVao);
    glDeleteBuffers(1, &bondVbo);
    glDeleteProgram(bondShader);

    glDeleteVertexArrays(1, &gridVao);
    glDeleteBuffers(1, &gridLineVbo);
    glDeleteBuffers(1, &gridInstVbo);
    glDeleteProgram(gridShader);
}

void RendererGL::initQuadGL() {
    const float quad[] = {
        -1.f, -1.f,  1.f, -1.f,  1.f,  1.f,
        -1.f, -1.f,  1.f,  1.f, -1.f,  1.f,
    };
    glGenBuffers(1, &quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
}

void RendererGL::initAtomGL() {
    glGenVertexArrays(1, &atomVao);
    glBindVertexArray(atomVao);

    // quad (location 0)
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenBuffers(1, &atomVbo);
    glBindBuffer(GL_ARRAY_BUFFER, atomVbo);

    glBindVertexArray(0);
}

void RendererGL::initBoxGL() {
    glGenVertexArrays(1, &boxVao);
    glGenBuffers(1, &boxVbo);
    glBindVertexArray(boxVao);
    glBindBuffer(GL_ARRAY_BUFFER, boxVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
}

void RendererGL::initBondGL() {
    glGenVertexArrays(1, &bondVao);
    glGenBuffers(1, &bondVbo);
    glBindVertexArray(bondVao);
    glBindBuffer(GL_ARRAY_BUFFER, bondVbo);

    const GLsizei stride = sizeof(BondInstance);

    // Location 0: Position A
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(BondInstance, posA));
    glVertexAttribDivisor(0, 1);

    // Location 1: Position B
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(BondInstance, posB));
    glVertexAttribDivisor(1, 1);

    // Location 2: Radius
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(BondInstance, radius));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
}

void RendererGL::initGridGL() {
    const float lines[] = {
        0,0,0, 1,0,0,   1,0,0, 1,1,0,   1,1,0, 0,1,0,   0,1,0, 0,0,0,
        0,0,1, 1,0,1,   1,0,1, 1,1,1,   1,1,1, 0,1,1,   0,1,1, 0,0,1,
        0,0,0, 0,0,1,   1,0,0, 1,0,1,   1,1,0, 1,1,1,   0,1,0, 0,1,1,
    };

    glGenVertexArrays(1, &gridVao);
    glGenBuffers(1, &gridLineVbo);
    glGenBuffers(1, &gridInstVbo);
    glBindVertexArray(gridVao);

    glBindBuffer(GL_ARRAY_BUFFER, gridLineVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribDivisor(0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, gridInstVbo);
    const GLsizei gs = sizeof(GridInstance);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, gs,
                          (void*)offsetof(GridInstance, origin));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, gs,
                          (void*)offsetof(GridInstance, cellSize));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, gs,
                        (void*)offsetof(GridInstance, atomCount));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
}

void RendererGL::initAtomColors() {
    const int typeCount = static_cast<int>(AtomData::Type::COUNT);
    std::vector<glm::vec3> typeColors(typeCount);
    for (int i = 0; i < typeCount; ++i) {
        const auto& props = AtomData::getProps(static_cast<AtomData::Type>(i));
        typeColors[i] = glm::vec3(
            props.color.r / 255.f,
            props.color.g / 255.f,
            props.color.b / 255.f
        );
    }
    for (GLuint shader : atomShaders) {
        if (shader == 0) continue;
        glUseProgram(shader);
        glUniform3fv(glGetUniformLocation(shader, "typeColors"),
                    typeCount, glm::value_ptr(typeColors[0]));
    }
    glUseProgram(0);
}

GLuint RendererGL::atomShaderForMode(SpeedColorMode mode) const {
    const size_t idx = static_cast<size_t>(mode);
    if (idx < atomShaders.size() && atomShaders[idx] != 0) {
        return atomShaders[idx];
    }
    return atomShaders[0];
}

// --------------------------------------------------------------- shaders ---

GLuint RendererGL::loadShader(GLenum type, std::string_view path, std::string_view defines) {
    std::ifstream file(path.data());
    if (!file.is_open()) {
        std::cerr << "Failed to open shader: " << path << '\n';
        return 0;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    if (!defines.empty()) {
        const size_t lineEnd = source.find('\n');
        if (lineEnd != std::string::npos && source.rfind("#version", 0) == 0) {
            source.insert(lineEnd + 1, std::string(defines) + "\n");
        } else {
            source = std::string(defines) + "\n" + source;
        }
    }

    return compileShader(type, source);
}

GLuint RendererGL::compileShader(GLenum type, std::string_view src) {
    const char* c = src.data();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &c, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << '\n';
    }
    return shader;
}

GLuint RendererGL::linkProgram(std::string_view vert, std::string_view frag, std::string_view geom, std::string_view vertDefines) {
    GLuint v = loadShader(GL_VERTEX_SHADER,   vert, vertDefines);
    GLuint f = loadShader(GL_FRAGMENT_SHADER, frag);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);

    GLuint g = 0;
    if (!geom.empty()) {
        g = loadShader(GL_GEOMETRY_SHADER, geom);
        glAttachShader(prog, g);
    }

    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Shader link error: " << log << '\n';
        glDeleteProgram(prog);
        prog = 0;
    }

    glDeleteShader(v);
    glDeleteShader(f);
    if (g) glDeleteShader(g);

    return prog;
}

// ------------------------------------------------------------------ draw ---

void RendererGL::drawShot(const AtomStorage& atoms, const SimBox& box)
{
    PROFILE_SCOPE("RendererGL::drawShot");
    currentBox = &box;
    updateMatrices();

    target.setActive(true);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.13f, 0.13f, 0.13f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (drawBonds) drawBondsGL();
    if (drawGrid)  drawGridGL(box.grid);
    drawBox(box);

    drawAtoms(atoms, box);
}

void RendererGL::drawAtoms(const AtomStorage& atoms, const SimBox& box) {
    PROFILE_SCOPE("RendererGL::drawAtoms");
    const size_t atomCount = atoms.size();
    const bool useSpeedGradient = speedColorMode != SpeedColorMode::AtomColor;
    atomShader = atomShaderForMode(speedColorMode);

    // --- maxSpeedSqr ---
    float maxSpeedSqr = 1.f;
    if (useSpeedGradient) {
        if (speedGradientMax > 0.0f) {
            maxSpeedSqr = speedGradientMax * speedGradientMax;
        } else {
            for (size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
                maxSpeedSqr = std::max(maxSpeedSqr, static_cast<float>(atoms.vel(atomIndex).sqrAbs()));
            }
        }
        if (maxSpeedSqr < 1e-6f) maxSpeedSqr = 1.f;
    }

    radii.resize(atomCount);
    for (size_t i = 0; i < atomCount; ++i) {
        const auto& props = AtomData::getProps(atoms.type(i));
        radii[i] = static_cast<float>(props.radius);
    }

    glUseProgram(atomShader);
    glUniformMatrix4fv(glGetUniformLocation(atomShader, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(atomShader, "view"),
                       1, GL_FALSE, glm::value_ptr(view));
    glUniform1i(glGetUniformLocation(atomShader, "colorMode"),   static_cast<int>(speedColorMode));
    glUniform1f(glGetUniformLocation(atomShader, "maxSpeedSqr"), maxSpeedSqr);

    if (useLighting()) {
        const glm::vec3 light = getLightDir();
        glUniform3f(glGetUniformLocation(atomShader, "lightDir"),
                    light.x, light.y, light.z);
    }

    const GLsizeiptr floatN    = atomCount * sizeof(float);
    const GLsizeiptr uint8N    = atomCount * sizeof(uint8_t);
    const GLsizeiptr totalSize = floatN * 7 + uint8N * 2;

    const GLsizeiptr offX        = 0;
    const GLsizeiptr offY        = offX      + floatN;
    const GLsizeiptr offZ        = offY      + floatN;
    const GLsizeiptr offRadius   = offZ      + floatN;
    const GLsizeiptr offVelX     = offRadius + floatN;
    const GLsizeiptr offVelY     = offVelX   + floatN;
    const GLsizeiptr offVelZ     = offVelY   + floatN;
    const GLsizeiptr offType     = offVelZ   + floatN;
    const GLsizeiptr offSelected = offType   + uint8N;

    if (selectedDataBuffer.size() < atomCount) {
        selectedDataBuffer.resize(atomCount);
    }
    std::fill_n(selectedDataBuffer.data(), selectedDataBuffer.size(), 0);
    const auto& selectedIndices = ToolsManager::pickingSystem->getSelectedIndices();
    for (const size_t idx : selectedIndices) {
        selectedDataBuffer[idx] = 1;
    }

    glBindBuffer(GL_ARRAY_BUFFER, atomVbo);
    glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, offX,        floatN, atoms.xData());
    glBufferSubData(GL_ARRAY_BUFFER, offY,        floatN, atoms.yData());
    glBufferSubData(GL_ARRAY_BUFFER, offZ,        floatN, atoms.zData());
    if (useSpeedGradient) {
        glBufferSubData(GL_ARRAY_BUFFER, offVelX,     floatN, atoms.vxData());
        glBufferSubData(GL_ARRAY_BUFFER, offVelY,     floatN, atoms.vyData());
        glBufferSubData(GL_ARRAY_BUFFER, offVelZ,     floatN, atoms.vzData());
    }    
    glBufferSubData(GL_ARRAY_BUFFER, offType,     uint8N, atoms.atomTypeData());
    glBufferSubData(GL_ARRAY_BUFFER, offRadius,   floatN, radii.data());
    glBufferSubData(GL_ARRAY_BUFFER, offSelected, uint8N, selectedDataBuffer.data());

    glBindVertexArray(atomVao);

    if (atomCount != lastAtomCount) {
        glBindBuffer(GL_ARRAY_BUFFER, atomVbo);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)offX);
        glVertexAttribDivisor(1, 1);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, (void*)offY);
        glVertexAttribDivisor(2, 1);

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, (void*)offZ);
        glVertexAttribDivisor(3, 1);

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 0, (void*)offRadius);
        glVertexAttribDivisor(4, 1);

        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 0, (void*)offVelX);
        glVertexAttribDivisor(5, 1);

        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 0, (void*)offVelY);
        glVertexAttribDivisor(6, 1);

        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 0, (void*)offVelZ);
        glVertexAttribDivisor(7, 1);

        glEnableVertexAttribArray(8);
        glVertexAttribIPointer(8, 1, GL_UNSIGNED_BYTE, 0, (void*)offType);
        glVertexAttribDivisor(8, 1);

        glEnableVertexAttribArray(9);
        glVertexAttribIPointer(9, 1, GL_UNSIGNED_BYTE, 0, (void*)offSelected);
        glVertexAttribDivisor(9, 1);

        lastAtomCount = atomCount;
    }

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(atomCount));
    glBindVertexArray(0);
}

void RendererGL::drawBox(const SimBox& box) {
    PROFILE_SCOPE("RendererGL::drawBox");
    const float x0 = 0, y0 = 0, z0 = 0;
    const float x1 = box.size.x,   y1 = box.size.y,   z1 = box.size.z;

    const float lines[] = {
        x0,y0,z0, x1,y0,z0,  x1,y0,z0, x1,y1,z0,
        x1,y1,z0, x0,y1,z0,  x0,y1,z0, x0,y0,z0,
        x0,y0,z1, x1,y0,z1,  x1,y0,z1, x1,y1,z1,
        x1,y1,z1, x0,y1,z1,  x0,y1,z1, x0,y0,z1,
        x0,y0,z0, x0,y0,z1,  x1,y0,z0, x1,y0,z1,
        x1,y1,z0, x1,y1,z1,  x0,y1,z0, x0,y1,z1,
    };

    glBindVertexArray(boxVao);
    glBindBuffer(GL_ARRAY_BUFFER, boxVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_DYNAMIC_DRAW);

    glUseProgram(boxShader);
    glUniformMatrix4fv(glGetUniformLocation(boxShader, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(boxShader, "view"),
                       1, GL_FALSE, glm::value_ptr(view));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_LINES, 0, 24);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void RendererGL::drawBondsGL() {
    PROFILE_SCOPE("RendererGL::drawBondsGL");
    if (bondShader == 0 || !atomStorage) return;

    bondData.clear();
    bondData.reserve(Bond::bonds_list.size());

    for (const Bond& bond : Bond::bonds_list) {
        if (bond.aIndex >= atomStorage->size() || bond.bIndex >= atomStorage->size()) {
            continue;
        }
        const size_t aIndex = bond.aIndex;
        const size_t bIndex = bond.bIndex;
        const Vec3f aPos = atomStorage->pos(aIndex);
        const Vec3f bPos = atomStorage->pos(bIndex);
        const float r = (AtomData::getProps(atomStorage->type(aIndex)).radius +
                         AtomData::getProps(atomStorage->type(bIndex)).radius) * 0.15f;
        bondData.emplace_back(
            glm::vec3(aPos.x, aPos.y, aPos.z),
            glm::vec3(bPos.x, bPos.y, bPos.z),
            r
        );
    }

    if (bondData.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, bondVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 bondData.size() * sizeof(BondInstance),
                 bondData.data(), GL_DYNAMIC_DRAW);

    glUseProgram(bondShader);
    glUniformMatrix4fv(glGetUniformLocation(bondShader, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(bondShader, "view"),
                       1, GL_FALSE, glm::value_ptr(view));

    const glm::vec3 light = getLightDir();
    glUniform3f(glGetUniformLocation(bondShader, "lightDir"),
                light.x, light.y, light.z);
    glUniform3f(glGetUniformLocation(bondShader, "bondColor"),
                0.2f, 0.4f, 0.9f);

    glBindVertexArray(bondVao);
    glDrawArraysInstanced(GL_POINTS, 0, 1,
                          static_cast<GLsizei>(bondData.size()));
    glBindVertexArray(0);
}

void RendererGL::drawGridGL(const SpatialGrid& grid) {
    PROFILE_SCOPE("RendererGL::drawGridGL");
    gridData.clear();

    int maxCount = 1;

    // Grid has a 1-cell border on each side; draw only interior cells in world coordinates.
    for (int z = 1; z < grid.sizeZ - 1; ++z) {
        for (int y = 1; y < grid.sizeY - 1; ++y) {
            for (int x = 1; x < grid.sizeX - 1; ++x) {
                const int countAtomsInCell = grid.countAtomsInCell(x, y, z);
                if (countAtomsInCell == 0) continue;
                gridData.emplace_back(
                    glm::vec3((x - 1) * grid.cellSize,
                              (y - 1) * grid.cellSize,
                              (z - 1) * grid.cellSize),
                    static_cast<float>(grid.cellSize),
                    static_cast<float>(countAtomsInCell)
                );
                maxCount = std::max(maxCount, countAtomsInCell);
            }
        }
    }

    if (gridData.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, gridInstVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 gridData.size() * sizeof(GridInstance),
                 gridData.data(), GL_DYNAMIC_DRAW);

    glUseProgram(gridShader);
    glUniformMatrix4fv(glGetUniformLocation(gridShader, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(gridShader, "view"),
                       1, GL_FALSE, glm::value_ptr(view));
    glUniform1f(glGetUniformLocation(gridShader, "uMaxCount"), float(maxCount));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(gridVao);
    glDrawArraysInstanced(GL_LINES, 0, 24,
                          static_cast<GLsizei>(gridData.size()));
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}
