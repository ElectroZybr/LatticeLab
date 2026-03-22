#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer3D.h"
#include "imgui-SFML.h"

Renderer3D::Renderer3D(sf::RenderTarget& t, sf::View& gv, sf::View& uv)
    : IRenderer(gv), target(t), uiView(uv)
{
    gladLoadGL();
    camera.setOrbitMode(true);
    initGL();
    initShaders();
}

Renderer3D::~Renderer3D() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &quadVbo);
    glDeleteBuffers(1, &instanceVbo);
    glDeleteProgram(shaderProgram);

    glDeleteVertexArrays(1, &boxVao);
    glDeleteBuffers(1, &boxVbo);
    glDeleteProgram(boxShader);
}

void Renderer3D::initGL() {
    // billboard квад — два треугольника
    const float quad[] = {
        -1.f, -1.f,
         1.f, -1.f,
         1.f,  1.f,
        -1.f, -1.f,
         1.f,  1.f,
        -1.f,  1.f,
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // квад (location 0)
    glGenBuffers(1, &quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // инстансинг буфер (locations 1,2,3)
    glGenBuffers(1, &instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);

    const GLsizei stride = sizeof(AtomInstance);

    // pos (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(AtomInstance, pos));
    glVertexAttribDivisor(1, 1);

    // radius (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(AtomInstance, radius));
    glVertexAttribDivisor(2, 1);

    // color (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(AtomInstance, color));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);

    initBoxGL();
}

void Renderer3D::initBoxGL() {
    glGenVertexArrays(1, &boxVao);
    glGenBuffers(1, &boxVbo);
    glBindVertexArray(boxVao);
    glBindBuffer(GL_ARRAY_BUFFER, boxVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    GLuint vert = loadShader(GL_VERTEX_SHADER,   "Rendering/3d/shaders/box.vert");
    GLuint frag = loadShader(GL_FRAGMENT_SHADER, "Rendering/3d/shaders/box.frag");
    boxShader = glCreateProgram();
    glAttachShader(boxShader, vert);
    glAttachShader(boxShader, frag);
    glLinkProgram(boxShader);
    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Renderer3D::drawBox(const SimBox& box) {
    const float x0 = box.start.x, y0 = box.start.y, z0 = box.start.z;
    const float x1 = box.end.x,   y1 = box.end.y,   z1 = box.end.z;

    // 12 рёбер куба = 24 вершины
    const float lines[] = {
        x0,y0,z0, x1,y0,z0,
        x1,y0,z0, x1,y1,z0,
        x1,y1,z0, x0,y1,z0,
        x0,y1,z0, x0,y0,z0,

        x0,y0,z1, x1,y0,z1,
        x1,y0,z1, x1,y1,z1,
        x1,y1,z1, x0,y1,z1,
        x0,y1,z1, x0,y0,z1,

        x0,y0,z0, x0,y0,z1,
        x1,y0,z0, x1,y0,z1,
        x1,y1,z0, x1,y1,z1,
        x0,y1,z0, x0,y1,z1,
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

GLuint Renderer3D::loadShader(GLenum type, std::string_view path) {
    std::ifstream file(path.data());
    if (!file.is_open()) {
        std::cerr << "Failed to open shader: " << path << '\n';
        return 0;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return compileShader(type, ss.str());
}

GLuint Renderer3D::compileShader(GLenum type, std::string_view src) {
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

void Renderer3D::initShaders() {
    GLuint vert = loadShader(GL_VERTEX_SHADER,   "Rendering/3d/shaders/atom.vert");
    GLuint frag = loadShader(GL_FRAGMENT_SHADER, "Rendering/3d/shaders/atom.frag");

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);

    GLint ok = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        std::cerr << "Shader link error: " << log << '\n';
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Renderer3D::updateMatrices() {
    const auto size = target.getSize();
    const float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);

    projection = glm::perspective(glm::radians(45.f), aspect, 0.1f, 1000.f);

    view = camera.getViewMatrix();
}

void Renderer3D::drawShot(const std::vector<Atom>& atoms,
                           const SimBox& box, float deltaTime)
{
    updateMatrices();

    // собираем инстансинг данные
    instanceData.clear();
    instanceData.reserve(atoms.size());

    const glm::vec3 boxOffset(
        static_cast<float>(box.start.x),
        static_cast<float>(box.start.y),
        static_cast<float>(box.start.z)
    );

    for (const Atom& atom : atoms) {
        sf::Color c = atom.getProps().color;
        instanceData.push_back({
            glm::vec3(atom.coords.x, atom.coords.y, atom.coords.z) + boxOffset,
            static_cast<float>(atom.getProps().radius),
            glm::vec3(c.r / 255.f, c.g / 255.f, c.b / 255.f)
        });
    }

    // рисуем через OpenGL
    target.setActive(true);

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.13f, 0.13f, 0.13f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    // uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"),
                       1, GL_FALSE, glm::value_ptr(view));

    glm::vec3 eye = camera.getEyePosition();
    glm::vec3 lightDirView = glm::normalize(
        glm::vec3(view * glm::vec4(eye, 0.0f)) + glm::vec3(25.f, 25.f, 0.f)
    );

    glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"),
                lightDirView.x, lightDirView.y, lightDirView.z);

    // загружаем данные атомов
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 instanceData.size() * sizeof(AtomInstance),
                 instanceData.data(), GL_DYNAMIC_DRAW);

    // рисуем
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6,
                          static_cast<GLsizei>(instanceData.size()));
    glBindVertexArray(0);

    drawBox(box);

    // ImGui поверх
    target.setActive(true);
    target.pushGLStates();
    target.setView(uiView);
    ImGui::SFML::Render(target);
    target.popGLStates();
}

void Renderer3D::setSelectionFrame(Vec2D start, Vec2D end, float scale) {
    // TODO: реализовать для 3D
}

void Renderer3D::setLassoContour(const std::vector<Vec2D>& points, float scale) {
    // TODO: реализовать для 3D
}

void Renderer3D::wallImage(Vec3D start, Vec3D end) {
    // TODO: реализовать для 3D
}

void Renderer3D::showSelectionFrame(bool show) {
    // TODO: реализовать для 3D
}

void Renderer3D::showLassoContour(bool show) {
    // TODO: реализовать для 3D
}
