#include "RendererBGFX.h"

#include <cstring>
#include <format>
#include <fstream>
#include <ranges>
#include <string_view>

#include "App/interaction/ToolsManager.h"
#include "Rendering/BgfxContext.h"

namespace {
    // в случае замены надо поменять atom2d.vert и atom3d.vert
    static constexpr uint16_t kTBOWidth = 4096;

    void ensureTBO(bgfx::TextureHandle& handle, uint32_t count) {
        if (bgfx::isValid(handle)) {
            bgfx::destroy(handle);
        }
        const uint16_t height = (count + kTBOWidth - 1) / kTBOWidth;
        handle = bgfx::createTexture2D(kTBOWidth, height, false, 1, bgfx::TextureFormat::R32F,
                                       BGFX_TEXTURE_NONE | BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP);
    }

    void uploadTBO(bgfx::TextureHandle handle, const float* data, uint32_t count) {
        const uint32_t fullRows = count / kTBOWidth;
        const uint32_t remainder = count % kTBOWidth;

        if (fullRows > 0) {
            bgfx::updateTexture2D(handle, 0, 0, 0, 0, kTBOWidth, fullRows, bgfx::makeRef(data, fullRows * kTBOWidth * sizeof(data[0])));
        }
        if (remainder > 0) {
            bgfx::updateTexture2D(handle, 0, 0, 0, fullRows, remainder, 1,
                                  bgfx::makeRef(data + fullRows * kTBOWidth, remainder * sizeof(data[0])));
        }
    }
}

bgfx::ProgramHandle RendererBGFX::loadProgram(std::string_view vsPath, std::string_view fsPath) {
    auto loadShader = [](std::string_view path) -> bgfx::ShaderHandle {
        std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error(std::format("Cannot open shader: {}", path));
        }

        const std::streamsize size = file.tellg();
        file.seekg(0);

        const bgfx::Memory* mem = bgfx::alloc(uint32_t(size) + 1);
        file.read(reinterpret_cast<char*>(mem->data), size);
        mem->data[size] = '\0';

        bgfx::ShaderHandle sh = bgfx::createShader(mem);
        bgfx::setName(sh, path.data());
        return sh;
    };

    bgfx::ShaderHandle vs = loadShader(vsPath);
    bgfx::ShaderHandle fs = loadShader(fsPath);
    return bgfx::createProgram(vs, fs, true);
}

RendererBGFX::RendererBGFX(sf::RenderTarget& t, sf::View& gv, SimBox& simbox) : IRenderer(gv, simbox), target(t) {
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x212121ff, 1.0f, 0);

    uLightDir = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
    uTypeColors = bgfx::createUniform("u_typeColors", bgfx::UniformType::Vec4, static_cast<int>(AtomData::Type::COUNT));
    uMaxSpeedSqr = bgfx::createUniform("u_maxSpeedSqr", bgfx::UniformType::Vec4);
    uMaxCount = bgfx::createUniform("u_maxCount", bgfx::UniformType::Vec4);
    uColorMode = bgfx::createUniform("u_colorMode", bgfx::UniformType::Vec4);

    initAtomBuffers();
    initBondBuffers();
    initBoxBuffers();
    initGridBuffers();
    initAtomColors();
}

RendererBGFX::~RendererBGFX() {
    bgfx::destroy(uLightDir);
    bgfx::destroy(uTypeColors);
    bgfx::destroy(uMaxSpeedSqr);
    bgfx::destroy(uMaxCount);
    bgfx::destroy(uColorMode);

    bgfx::destroy(atomQuadVbh);
    bgfx::destroy(atomProgram);

    bgfx::destroy(bondVbh);
    bgfx::destroy(bondProgram);

    bgfx::destroy(boxVbh);
    bgfx::destroy(boxProgram);

    bgfx::destroy(gridLineVbh);
    bgfx::destroy(gridProgram);
}

// Init

void RendererBGFX::initAtomColors() {
    const int typeCount = static_cast<int>(AtomData::Type::COUNT);
    typeColorsData.resize(typeCount);

    for (int i = 0; i < typeCount; ++i) {
        const auto& props = AtomData::getProps(static_cast<AtomData::Type>(i));
        typeColorsData[i] = glm::vec4(props.color.r / 255.f, props.color.g / 255.f, props.color.b / 255.f, 1.f);
    }
}

void RendererBGFX::initAtomBuffers() {
    const float quad[] = {
        -1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f,
    };

    bgfx::VertexLayout quadLayout;
    quadLayout.begin().add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float).end();

    atomQuadVbh = bgfx::createVertexBuffer(bgfx::copy(quad, sizeof(quad)), quadLayout);

    sTboX = bgfx::createUniform("s_posX", bgfx::UniformType::Sampler);
    sTboY = bgfx::createUniform("s_posY", bgfx::UniformType::Sampler);
    sTboZ = bgfx::createUniform("s_posZ", bgfx::UniformType::Sampler);
    sTboVX = bgfx::createUniform("s_velX", bgfx::UniformType::Sampler);
    sTboVY = bgfx::createUniform("s_velY", bgfx::UniformType::Sampler);
    sTboVZ = bgfx::createUniform("s_velZ", bgfx::UniformType::Sampler);
    sTboType = bgfx::createUniform("s_type", bgfx::UniformType::Sampler);
    sTboRadius = bgfx::createUniform("s_radius", bgfx::UniformType::Sampler);
    sTboSel = bgfx::createUniform("s_sel", bgfx::UniformType::Sampler);
}

void RendererBGFX::initBoxBuffers() {
    bgfx::VertexLayout layout;
    layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();
    boxVbh = bgfx::createDynamicVertexBuffer(24, layout, BGFX_BUFFER_ALLOW_RESIZE);
}

void RendererBGFX::initBondBuffers() {
    bgfx::VertexLayout layout;
    layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();

    bondVbh = bgfx::createDynamicVertexBuffer(1, layout, BGFX_BUFFER_ALLOW_RESIZE);
}

void RendererBGFX::initGridBuffers() {
    const float lines[] = {
        0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1,
        1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1,
    };

    bgfx::VertexLayout lineLayout;
    lineLayout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();

    gridLineVbh = bgfx::createVertexBuffer(bgfx::copy(lines, sizeof(lines)), lineLayout);
}

// Draw

void RendererBGFX::drawShot(const AtomStorage& atoms, const Bond::List& bonds, const SimBox& box) {
    updateMatrices();

    const auto size = target.getSize();
    bgfx::setViewRect(0, 0, 0, size.x, size.y);
    bgfx::setViewTransform(0, &view[0][0], &projection[0][0]);
    bgfx::touch(0);

    if (drawBonds) {
        drawBondsImpl(atoms, bonds);
    }
    if (drawGrid) {
        drawGridImpl(box.grid);
    }

    drawBoxImpl(box);
    drawAtomsImpl(atoms);
}

void RendererBGFX::drawAtomsImpl(const AtomStorage& atoms) {
    const size_t count = atoms.size();
    if (count == 0) {
        return;
    }

    if (count > tboCapacity_) {
        ensureTBO(tboX, count);
        ensureTBO(tboY, count);
        ensureTBO(tboZ, count);
        ensureTBO(tboVX, count);
        ensureTBO(tboVY, count);
        ensureTBO(tboVZ, count);
        ensureTBO(tboType, count);
        ensureTBO(tboRadius, count);
        ensureTBO(tboSel, count);
        tboCapacity_ = count;
    }

    uploadTBO(tboX, atoms.xData(), count);
    uploadTBO(tboY, atoms.yData(), count);
    uploadTBO(tboZ, atoms.zData(), count);
    uploadTBO(tboVX, atoms.vxData(), count);
    uploadTBO(tboVY, atoms.vyData(), count);
    uploadTBO(tboVZ, atoms.vzData(), count);

    radii.resize(count);
    typeData.resize(count);
    selectedData.assign(count, 0.f);

    for (size_t i = 0; i < count; ++i) {
        const auto& props = AtomData::getProps(atoms.type(i));
        radii[i] = props.radius;
        typeData[i] = static_cast<float>(atoms.type(i));
    }
    for (const size_t idx : ToolsManager::pickingSystem->getSelectedIndices()) {
        selectedData[idx] = 1.f;
    }

    uploadTBO(tboRadius, radii.data(), count);
    uploadTBO(tboType, typeData.data(), count);
    uploadTBO(tboSel, selectedData.data(), count);

    // maxSpeedSqr
    float maxSpeedSqr = 1.f;
    if (speedColorMode != SpeedColorMode::AtomColor) {
        if (speedGradientMax > 0.f) {
            maxSpeedSqr = speedGradientMax * speedGradientMax;
        }
        else {
            const auto maxSpeedIt =
                std::ranges::max_element(std::views::iota(size_t{0}, count), {}, [&](size_t i) { return atoms.vel(i).sqrAbs(); });
            maxSpeedSqr = std::max(1e-6f, atoms.vel(*maxSpeedIt).sqrAbs());
        }
    }

    const glm::vec4 speedParam(maxSpeedSqr, 0, 0, 0);
    bgfx::setUniform(uMaxSpeedSqr, &speedParam);

    if (useLighting()) {
        const glm::vec4 lightDir(getLightDir(), 0.f);
        bgfx::setUniform(uLightDir, &lightDir);
    }

    const glm::vec4 colorModeVec(static_cast<float>(speedColorMode), 0, 0, 0);
    bgfx::setUniform(uColorMode, &colorModeVec);
    bgfx::setUniform(uTypeColors, typeColorsData.data(), typeColorsData.size());

    bgfx::setTexture(0, sTboX, tboX);
    bgfx::setTexture(1, sTboY, tboY);
    bgfx::setTexture(2, sTboZ, tboZ);
    bgfx::setTexture(3, sTboVX, tboVX);
    bgfx::setTexture(4, sTboVY, tboVY);
    bgfx::setTexture(5, sTboVZ, tboVZ);
    bgfx::setTexture(6, sTboType, tboType);
    bgfx::setTexture(7, sTboRadius, tboRadius);
    bgfx::setTexture(8, sTboSel, tboSel);

    bgfx::setVertexBuffer(0, atomQuadVbh);
    bgfx::setInstanceCount(count);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
    bgfx::submit(0, atomProgram);
}

void RendererBGFX::drawBoxImpl(const SimBox& box) {
    const float x1 = box.size.x, y1 = box.size.y, z1 = box.size.z;

    const float lines[] = {
        0, y1, 0, x1, y1, 0, x1, y1, 0, x1, y1, z1, x1, y1, z1, 0,  y1, z1, 0, y1, z1, 0, y1, 0,
        0, 0,  0, x1, 0,  0, x1, 0,  0, x1, 0,  z1, x1, 0,  z1, 0,  0,  z1, 0, 0,  z1, 0, 0,  0,
        0, 0,  0, 0,  y1, 0, x1, 0,  0, x1, y1, 0,  x1, 0,  z1, x1, y1, z1, 0, 0,  z1, 0, y1, z1,
    };
    const bgfx::Memory* mem = bgfx::copy(lines, sizeof(lines));
    bgfx::update(boxVbh, 0, mem);

    bgfx::setVertexBuffer(0, boxVbh);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_PT_LINES);

    bgfx::submit(0, boxProgram);
}

void RendererBGFX::drawBondsImpl(const AtomStorage& atoms, const Bond::List& bonds) {
    if (bonds.empty()) {
        return;
    }

    std::vector<glm::vec3> verts;
    verts.reserve(bonds.size() * 2);

    for (const Bond& bond : bonds) {
        if (bond.aIndex >= atoms.size() || bond.bIndex >= atoms.size()) {
            continue;
        }
        const Vec3f a = atoms.pos(bond.aIndex);
        const Vec3f b = atoms.pos(bond.bIndex);
        verts.emplace_back(a.x, a.y, a.z);
        verts.emplace_back(b.x, b.y, b.z);
    }

    if (verts.empty()) {
        return;
    }

    const bgfx::Memory* mem = bgfx::copy(verts.data(), verts.size() * sizeof(verts[0]));
    bgfx::update(bondVbh, 0, mem);

    bgfx::setVertexBuffer(0, bondVbh);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_PT_LINES);

    bgfx::submit(0, bondProgram);
}

void RendererBGFX::drawGridImpl(const SpatialGrid& grid) {
    gridData.clear();

    int maxCount = 1;
    for (int z = 1; z < grid.sizeZ - 1; ++z) {
        for (int y = 1; y < grid.sizeY - 1; ++y) {
            for (int x = 1; x < grid.sizeX - 1; ++x) {
                const int count = grid.countAtomsInCell(x, y, z);
                if (count > 0) {
                    gridData.emplace_back(glm::vec3((x - 1) * grid.cellSize, (y - 1) * grid.cellSize, (z - 1) * grid.cellSize),
                                          grid.cellSize, count);
                    maxCount = std::max(maxCount, count);
                }
            }
        }
    }

    if (gridData.empty()) {
        return;
    }

    const uint32_t numInstances = uint32_t(gridData.size());
    const uint16_t stride = sizeof(GridInstance);

    bgfx::InstanceDataBuffer idb;
    bgfx::allocInstanceDataBuffer(&idb, numInstances, stride);
    std::memcpy(idb.data, gridData.data(), numInstances * stride);

    const glm::vec4 maxCountVec(maxCount, 0, 0, 0);
    bgfx::setUniform(uMaxCount, &maxCountVec);

    bgfx::setVertexBuffer(0, gridLineVbh);
    bgfx::setInstanceDataBuffer(&idb);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_PT_LINES);
    bgfx::submit(0, gridProgram);
}