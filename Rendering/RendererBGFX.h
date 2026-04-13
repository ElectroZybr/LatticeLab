#pragma once

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
    static bgfx::ProgramHandle loadProgram(std::string_view vsPath, std::string_view fsPath);

    virtual void updateMatrices() = 0;
    virtual glm::vec3 getLightDir() = 0;
    virtual bool useLighting() = 0;

    sf::RenderTarget& target;
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

    // Vertex buffers
    bgfx::VertexBufferHandle atomQuadVbh = BGFX_INVALID_HANDLE;
    bgfx::DynamicVertexBufferHandle bondVbh = BGFX_INVALID_HANDLE;
    bgfx::DynamicVertexBufferHandle boxVbh = BGFX_INVALID_HANDLE;
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

    std::vector<BondVertex> bondData;
    std::vector<GridInstance> gridData;
    std::vector<glm::vec4> typeColorsData;

    // Atoms
    bgfx::UniformHandle sTboX = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sTboY = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sTboZ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sTboVX = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sTboVY = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sTboVZ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sTboType = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sTboRadius = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sTboSel = BGFX_INVALID_HANDLE;

    bgfx::TextureHandle tboX = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle tboY = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle tboZ = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle tboVX = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle tboVY = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle tboVZ = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle tboType = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle tboRadius = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle tboSel = BGFX_INVALID_HANDLE;

    std::vector<float> selectedData;
    std::vector<float> radii;
    std::vector<float> typeData;
    size_t tboCapacity_ = 0;
};
