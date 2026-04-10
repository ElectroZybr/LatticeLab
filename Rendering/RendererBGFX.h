#pragma once

#include <array>
#include <vector>

#include <SFML/Graphics.hpp>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <glm/glm.hpp>

#include "Rendering/BaseRenderer.h"

class RendererBGFX : public IRenderer {
public:
    RendererBGFX(sf::RenderTarget& t, sf::View& gv, SimBox& simbox);
    ~RendererBGFX() override;

    void drawShot(const AtomStorage& atoms, const Bond::List& bonds, const SimBox& box) override;

protected:
    virtual void updateMatrices() = 0;
    virtual glm::vec3 getLightDir() = 0;
    virtual bool useLighting() = 0;

    sf::RenderTarget& target;
    const SimBox* currentBox = nullptr;
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};

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
    std::array<bgfx::ProgramHandle, 3> atomPrograms{{BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE}};

    // Bond
    bgfx::DynamicVertexBufferHandle bondVbh = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle bondProgram = BGFX_INVALID_HANDLE;

    // Box
    bgfx::DynamicVertexBufferHandle boxVbh = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle boxProgram = BGFX_INVALID_HANDLE;

    // Grid
    bgfx::VertexBufferHandle gridLineVbh = BGFX_INVALID_HANDLE;
    bgfx::DynamicVertexBufferHandle gridInstVbh = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle gridProgram = BGFX_INVALID_HANDLE;

    // Uniforms
    bgfx::UniformHandle uProjection = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle uView = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle uLightDir = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle uTypeColors = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle uMaxSpeedSqr = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle uMaxCount = BGFX_INVALID_HANDLE;

    struct alignas(16) BondVertex {
        glm::vec3 posA;
        float pad0 = 0;
        glm::vec3 posB;
        float radius;
    };

    struct alignas(16) GridInstance {
        glm::vec3 origin;
        float cellSize;
        float atomCount;
        float pad[3] = {};
    };

    struct AtomInstance {
        float x, y, z;
        float radius;
        float vx, vy, vz;
        uint8_t type;
        uint8_t selected;
        uint8_t pad[2] = {};
    };

    std::vector<AtomInstance> atomInstData;
    std::vector<BondVertex> bondData;
    std::vector<GridInstance> gridData;
    std::vector<float> radii;
    std::vector<glm::vec4> typeColorsData;
};
