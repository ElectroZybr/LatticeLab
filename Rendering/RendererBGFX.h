#pragma once

#include <vector>

#include <SFML/Graphics.hpp>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <glm/glm.hpp>

#include "Rendering/BaseRenderer.h"

class RendererBGFX : public IRenderer {
public:
    RendererBGFX(sf::RenderTarget& t, sf::WindowHandle nativeHandle, sf::View& gv, SimBox& simbox);
    ~RendererBGFX() override;

    void drawShot(const AtomStorage& atoms, const Bond::List& bonds, const SimBox& box) override;

protected:
    static bgfx::ProgramHandle loadProgram(std::string_view vsPath, std::string_view fsPath);

    virtual void updateMatrices() = 0;
    virtual glm::vec3 getLightDir() = 0;
    virtual bool useLighting() = 0;

    sf::RenderTarget& target;
    const SimBox* currentBox = nullptr;
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};

    bgfx::ProgramHandle atomProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle bondProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle boxProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle gridProgram = BGFX_INVALID_HANDLE;

private:
    void initAtomBuffers();
    void initBondBuffers();
    void initBoxBuffers();
    void initGridBuffers();
    void initAtomColors();

    void drawAtomsImpl(const AtomStorage& atoms);
    void drawBondsImpl(const AtomStorage& atoms, const Bond::List& bonds);
    void drawBoxImpl(const SimBox& box);
    void drawGridImpl(const SpatialGrid& grid);

    // Atom
    bgfx::VertexBufferHandle atomQuadVbh = BGFX_INVALID_HANDLE;
    bgfx::DynamicVertexBufferHandle atomInstVbh = BGFX_INVALID_HANDLE;

    // Bond
    bgfx::DynamicVertexBufferHandle bondVbh = BGFX_INVALID_HANDLE;

    // Box
    bgfx::DynamicVertexBufferHandle boxVbh = BGFX_INVALID_HANDLE;

    // Grid
    bgfx::VertexBufferHandle gridLineVbh = BGFX_INVALID_HANDLE;

    // Uniforms
    bgfx::UniformHandle uLightDir = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle uTypeColors = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle uMaxSpeedSqr = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle uMaxCount = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle uColorMode = BGFX_INVALID_HANDLE;

    struct BondVertex {
        glm::vec3 posA;
        float pad0 = 0;

        glm::vec3 posB;
        float pad1 = 0;

        float radius, pad[3] = {};
    };

    struct GridInstance {
        glm::vec3 origin;
        float cellSize;
        float atomCount;
        float pad[3] = {};
    };

    struct AtomInstance {
        float x, y, z, radius;  // texcoord0
        float vx, vy, vz, type; // texcoord1
        float selected, pad[3];
    };

    std::vector<AtomInstance> atomInstData;
    std::vector<BondVertex> bondData;
    std::vector<GridInstance> gridData;
    std::vector<float> radii;
    std::vector<glm::vec4> typeColorsData;
};
