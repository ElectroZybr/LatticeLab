#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SFML/Window/Context.hpp>

#include "Engine/physics/Bond.h"
#include "RendererGL.h"

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

RendererGL::RendererGL(sf::RenderTarget& t, sf::View& gv)
    : IRenderer(gv), target(t)
{
    initializeGlad(t);
    initQuadGL();
    initAtomGL();
    initBoxGL();
    initBondGL();
    initGridGL();
}

RendererGL::~RendererGL() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &quadVbo);
    glDeleteBuffers(1, &instanceVbo);
    glDeleteProgram(shaderProgram);

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
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // quad (location 0)
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // instance VBO (location 1..4)
    glGenBuffers(1, &instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);

    const GLsizei stride = sizeof(AtomInstance);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(AtomInstance, pos));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(AtomInstance, radius));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(AtomInstance, color));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(AtomInstance, isSelected));
    glVertexAttribDivisor(4, 1);

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

// --------------------------------------------------------------- shaders ---

GLuint RendererGL::loadShader(GLenum type, std::string_view path) {
    std::ifstream file(path.data());
    if (!file.is_open()) {
        std::cerr << "Failed to open shader: " << path << '\n';
        return 0;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return compileShader(type, ss.str());
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

GLuint RendererGL::linkProgram(std::string_view vert, std::string_view frag,
                               std::string_view geom)
{
    GLuint v = loadShader(GL_VERTEX_SHADER,   vert);
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

void RendererGL::drawShot(const AtomStorage& atoms,
                          const SimBox& box)
{
    currentBox = &box;
    updateMatrices();

    const glm::vec3 boxOffset(box.start.x, box.start.y, box.start.z);

    // --- собираем instance-данные атомов ---
    instanceData.clear();
    instanceData.reserve(atoms.size());

    float maxSpeedSqr = 1.f;
    if (speedGradient) {
        if (speedGradientMax > 0.0f) {
            maxSpeedSqr = speedGradientMax * speedGradientMax;
        } else {
            for (std::size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
                maxSpeedSqr = std::max(maxSpeedSqr, static_cast<float>(atoms.vel(atomIndex).sqrAbs()));
            }
        }
        if (maxSpeedSqr < 1e-6f) {
            maxSpeedSqr = 1.f;
        }
    }

    for (std::size_t atomIndex = 0; atomIndex < atoms.size(); ++atomIndex) {
        const Vec3f pos = atoms.pos(atomIndex);
        const Vec3f vel = atoms.vel(atomIndex);
        const auto type = atoms.type(atomIndex);
        const auto& props = AtomData::getProps(type);

        sf::Color sfColor;
        if (speedGradient) {
            const float t = std::clamp(
                std::sqrt(static_cast<float>(vel.sqrAbs()) / maxSpeedSqr), 0.f, 1.f);
            sfColor = speedGradientTurbo
                ? turboColor(t)
                : sf::Color(255 * t, 0, (1.f - t) * 255.f);
        }
        else {
            sfColor = props.color;
        }

        glm::vec3 color = glm::vec3(sfColor.r / 255.f, sfColor.g / 255.f, sfColor.b / 255.f);

        instanceData.emplace_back(
            glm::vec3(pos.x, pos.y, pos.z) + boxOffset,
            color,
            static_cast<float>(props.radius),
            atoms.isSelected(atomIndex) ? 1.0f : 0.0f
        );
    }

    // --- GL ---
    target.setActive(true);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.13f, 0.13f, 0.13f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- атомы ---
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"),
                       1, GL_FALSE, glm::value_ptr(view));

    if (useLighting()) {
        const glm::vec3 light = getLightDir();
        glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"),
                    light.x, light.y, light.z);
    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 instanceData.size() * sizeof(AtomInstance),
                 instanceData.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6,
                          static_cast<GLsizei>(instanceData.size()));
    glBindVertexArray(0);

    if (drawBonds) drawBondsGL(boxOffset);
    if (drawGrid)  drawGridGL(box.grid, boxOffset);
    drawBox(box);
    drawOverlay();
}

void RendererGL::drawBox(const SimBox& box) {
    const float x0 = box.start.x, y0 = box.start.y, z0 = box.start.z;
    const float x1 = box.end.x,   y1 = box.end.y,   z1 = box.end.z;

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

void RendererGL::drawBondsGL(const glm::vec3& boxOffset) {
    if (bondShader == 0 || !atomStorage) return;

    bondData.clear();
    bondData.reserve(Bond::bonds_list.size());

    for (const Bond& bond : Bond::bonds_list) {
        if (bond.aIndex >= atomStorage->size() || bond.bIndex >= atomStorage->size()) {
            continue;
        }
        const std::size_t aIndex = bond.aIndex;
        const std::size_t bIndex = bond.bIndex;
        const Vec3f aPos = atomStorage->pos(aIndex);
        const Vec3f bPos = atomStorage->pos(bIndex);
        const float r = (AtomData::getProps(atomStorage->type(aIndex)).radius +
                         AtomData::getProps(atomStorage->type(bIndex)).radius) * 0.15f;
        bondData.emplace_back(
            glm::vec3(aPos.x, aPos.y, aPos.z) + boxOffset,
            glm::vec3(bPos.x, bPos.y, bPos.z) + boxOffset,
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

void RendererGL::drawGridGL(const SpatialGrid& grid, const glm::vec3& boxOffset) {
    gridData.clear();

    int maxCount = 1;

    for (int z = 0; z < grid.sizeZ; ++z) {
        for (int y = 0; y < grid.sizeY; ++y) {
            for (int x = 0; x < grid.sizeX; ++x) {
                auto cell = grid.atIndex(x, y, z);
                if (!cell || cell->empty()) continue;
                gridData.push_back({
                    glm::vec3(x * grid.cellSize,
                            y * grid.cellSize,
                            z * grid.cellSize) + boxOffset,
                    static_cast<float>(grid.cellSize),
                    static_cast<float>(cell->size())
                });
                maxCount = std::max(maxCount, (int)cell->size());
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
