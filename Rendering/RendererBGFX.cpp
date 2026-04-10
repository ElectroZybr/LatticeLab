#include "RendererBGFX.h"

#include <format>
#include <fstream>
#include <string_view>

#include "App/interaction/ToolsManager.h"

static bgfx::ProgramHandle loadProgram(std::string_view vsPath, std::string_view fsPath) {
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
    target.setActive(false);

    bgfx::renderFrame();

    bgfx::Init init;
    init.type = bgfx::RendererType::OpenGL;
    init.platformData.nwh = nullptr;
    init.platformData.ndt = nullptr;
    init.platformData.context = nullptr;

    const auto size = target.getSize();
    init.resolution.width = size.x;
    init.resolution.height = size.y;
    init.resolution.reset = BGFX_RESET_NONE;

    if (!bgfx::init(init)) {
        throw std::runtime_error("bgfx::init failed");
    }

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x212121ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, size.x, size.y);

    uProjection = bgfx::createUniform("u_projection", bgfx::UniformType::Mat4);
    uView = bgfx::createUniform("u_view", bgfx::UniformType::Mat4);
    uLightDir = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
    uTypeColors = bgfx::createUniform("u_typeColors", bgfx::UniformType::Vec4, static_cast<int>(AtomData::Type::COUNT));
    uMaxSpeedSqr = bgfx::createUniform("u_maxSpeedSqr", bgfx::UniformType::Vec4);
    uMaxCount = bgfx::createUniform("u_maxCount", bgfx::UniformType::Vec4);

    initAtomBuffers();
    initBondBuffers();
    initBoxBuffers();
    initGridBuffers();
    initAtomColors();
}

RendererBGFX::~RendererBGFX() {
    bgfx::destroy(uProjection);
    bgfx::destroy(uView);
    bgfx::destroy(uLightDir);
    bgfx::destroy(uTypeColors);
    bgfx::destroy(uMaxSpeedSqr);
    bgfx::destroy(uMaxCount);

    bgfx::destroy(atomQuadVbh);
    bgfx::destroy(atomInstVbh);
    for (auto& p : atomPrograms) {
        bgfx::destroy(p);
    }

    bgfx::destroy(bondVbh);
    bgfx::destroy(bondProgram);

    bgfx::destroy(boxVbh);
    bgfx::destroy(boxProgram);

    bgfx::destroy(gridLineVbh);
    bgfx::destroy(gridInstVbh);
    bgfx::destroy(gridProgram);

    bgfx::shutdown();
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

    const bgfx::Memory* mem = bgfx::copy(quad, sizeof(quad));
    atomQuadVbh = bgfx::createVertexBuffer(mem, quadLayout);

    bgfx::VertexLayout instLayout;
    instLayout.begin()
        .add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float) // x, y, z
        .add(bgfx::Attrib::TexCoord1, 1, bgfx::AttribType::Float) // radius
        .add(bgfx::Attrib::TexCoord2, 3, bgfx::AttribType::Float) // vx, vy, vz
        .add(bgfx::Attrib::TexCoord3, 1, bgfx::AttribType::Float) // type
        .add(bgfx::Attrib::TexCoord4, 1, bgfx::AttribType::Float) // selected
        .end();

    atomInstVbh = bgfx::createDynamicVertexBuffer(1, instLayout, BGFX_BUFFER_ALLOW_RESIZE);
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

    bgfx::VertexLayout instLayout;
    instLayout.begin()
        .add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float) // origin
        .add(bgfx::Attrib::TexCoord1, 1, bgfx::AttribType::Float) // cellSize
        .add(bgfx::Attrib::TexCoord2, 1, bgfx::AttribType::Float) // atomCount
        .end();

    gridInstVbh = bgfx::createDynamicVertexBuffer(1, instLayout, BGFX_BUFFER_ALLOW_RESIZE);
}

// Draw

void RendererBGFX::drawShot(const AtomStorage& atoms, const Bond::List& bonds, const SimBox& box) {
    currentBox = &box;
    updateMatrices();

    const auto size = target.getSize();
    bgfx::setViewRect(0, 0, 0, uint16_t(size.x), uint16_t(size.y));
    bgfx::touch(0);

    // bgfx::setUniform(uProjection, &projection[0][0]);
    // bgfx::setUniform(uView, &view[0][0]);

    // if (drawBonds) {
    //     drawBondsImpl(atoms, bonds);
    // }
    // if (drawGrid) {
    //     drawGridImpl(box.grid);
    // }

    // drawBoxImpl(box);
    // drawAtomsImpl(atoms);

    bgfx::frame();
}

void RendererBGFX::drawAtomsImpl(const AtomStorage& atoms) {
    const size_t count = atoms.size();
    if (count == 0) {
        return;
    }

    atomInstData.resize(count);
    for (size_t i = 0; i < count; ++i) {
        const Vec3f pos = atoms.pos(i);
        const Vec3f vel = atoms.vel(i);
        atomInstData[i] = {
            .x = float(pos.x),
            .y = float(pos.y),
            .z = float(pos.z),
            .radius = float(AtomData::getProps(atoms.type(i)).radius),
            .vx = float(vel.x),
            .vy = float(vel.y),
            .vz = float(vel.z),
            .type = static_cast<uint8_t>(atoms.type(i)),
            .selected = 0,
        };
    }

    // selected
    const auto& selectedIndices = ToolsManager::pickingSystem->getSelectedIndices();
    for (const size_t idx : selectedIndices) {
        if (idx < count) {
            atomInstData[idx].selected = 1;
        }
    }

    const bgfx::Memory* mem = bgfx::makeRef(atomInstData.data(), uint32_t(count * sizeof(atomInstData[0])));
    bgfx::update(atomInstVbh, 0, mem);

    // maxSpeedSqr
    float maxSpeedSqr = 1.f;
    if (speedColorMode != SpeedColorMode::AtomColor) {
        if (speedGradientMax > 0.f) {
            maxSpeedSqr = speedGradientMax * speedGradientMax;
        }
        else {
            for (size_t i = 0; i < count; ++i) {
                maxSpeedSqr = std::max(maxSpeedSqr, float(atoms.vel(i).sqrAbs()));
            }
        }
        if (maxSpeedSqr < 1e-6f) {
            maxSpeedSqr = 1.f;
        }
    }

    const glm::vec4 speedParam(maxSpeedSqr, 0, 0, 0);
    bgfx::setUniform(uMaxSpeedSqr, &speedParam);

    if (useLighting()) {
        const glm::vec3 ld = getLightDir();
        const glm::vec4 lightDir(ld, 0.f);
        bgfx::setUniform(uLightDir, &lightDir);
    }

    bgfx::setVertexBuffer(0, atomQuadVbh);
    bgfx::setVertexBuffer(1, atomInstVbh);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);

    const size_t modeIdx = static_cast<size_t>(speedColorMode);
    bgfx::setUniform(uTypeColors, typeColorsData.data(), uint16_t(typeColorsData.size()));
    bgfx::submit(0, atomPrograms[modeIdx < 3 ? modeIdx : 0]);
}

void RendererBGFX::drawBoxImpl(const SimBox& box) {
    const float x1 = box.size.x, y1 = box.size.y, z1 = box.size.z;

    const float lines[] = {
        0, y1, 0, x1, y1, 0, x1, y1, 0, x1, y1, z1, x1, y1, z1, 0,  y1, z1, 0, y1, z1, 0, y1, 0,
        0, 0,  0, x1, 0,  0, x1, 0,  0, x1, 0,  z1, x1, 0,  z1, 0,  0,  z1, 0, 0,  z1, 0, 0,  0,
        0, 0,  0, 0,  y1, 0, x1, 0,  0, x1, y1, 0,  x1, 0,  z1, x1, y1, z1, 0, 0,  z1, 0, y1, z1,
    };

    const bgfx::Memory* mem = bgfx::makeRef(lines, sizeof(lines));
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

    const bgfx::Memory* mem = bgfx::makeRef(verts.data(), uint32_t(verts.size() * sizeof(verts[0])));
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
                const int cnt = grid.countAtomsInCell(x, y, z);
                if (cnt == 0) {
                    continue;
                }
                gridData.emplace_back(glm::vec3((x - 1) * grid.cellSize, (y - 1) * grid.cellSize, (z - 1) * grid.cellSize),
                                      float(grid.cellSize), float(cnt));
                maxCount = std::max(maxCount, cnt);
            }
        }
    }

    if (gridData.empty()) {
        return;
    }

    bgfx::update(gridInstVbh, 0, bgfx::makeRef(gridData.data(), uint32_t(gridData.size() * sizeof(gridData[0]))));

    const glm::vec4 maxCountVec(float(maxCount), 0, 0, 0);
    bgfx::setUniform(uMaxCount, &maxCountVec);

    bgfx::setVertexBuffer(0, gridLineVbh);
    bgfx::setVertexBuffer(1, gridInstVbh);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_PT_LINES);

    bgfx::submit(0, gridProgram);
}
