#include "AppActions.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <glm/gtc/constants.hpp>
#include "App/AppSignals.h"
#include "Lattice/Generators/HexTriangleBipyramid.h"
#include "Lattice/Generators/LatticeFill.hpp"
#include "Lattice/Generators/RandomFill.hpp"
#include "Lattice/Generators/Region.hpp"
#include "App/capture/CaptureOutputPath.h"
#include "App/capture/CaptureController.h"
#include "App/interaction/ToolsManager.h"
#include "App/viewport/SceneViewport.h"
#include "App/save_system/AppStateIO.h"
#include "Lattice/Engine/Simulation.h"
#include "GUI/interface/UiState.h"

namespace {
    glm::vec3 clampBoxSize(glm::vec3 size) {
        return glm::max(size, glm::vec3(1.0f));
    }

    glm::vec3 moveTowards(glm::vec3 current, glm::vec3 target, float maxStep) {
        const glm::vec3 delta = target - current;
        return glm::vec3(
            std::abs(delta.x) <= maxStep ? target.x : current.x + std::copysign(maxStep, delta.x),
            std::abs(delta.y) <= maxStep ? target.y : current.y + std::copysign(maxStep, delta.y),
            std::abs(delta.z) <= maxStep ? target.z : current.z + std::copysign(maxStep, delta.z)
        );
    }

    void shiftAtoms(AtomStorage& atomStorage, glm::vec3 delta) {
        float* x = atomStorage.xData();
        float* y = atomStorage.yData();
        float* z = atomStorage.zData();
        const size_t atomCount = atomStorage.size();

        for (size_t i = 0; i < atomCount; ++i) {
            x[i] += delta.x;
            y[i] += delta.y;
            z[i] += delta.z;
        }
    }

    void applyResizeBox(Lattice::Simulation& simulation, const glm::vec3& newSize) {
        World& world = simulation.world();
        const glm::vec3 clampedSize = clampBoxSize(newSize);
        const glm::vec3 oldSize = world.getWorldSize();
        const glm::vec3 delta = (clampedSize - oldSize) * 0.5f;

        shiftAtoms(simulation.atoms(), delta);
        world.setRenderOffset(world.getRenderOffset() - delta);
        simulation.setSizeBox(clampedSize);
    }

    void toggleXYZRecording(CaptureController& captureController, Lattice::Simulation& simulation) {
        if (simulation.isXYZRecording()) {
            simulation.stopXYZRecording();
            return;
        }

        std::error_code fsError;
        const std::filesystem::path outputDirectory = captureController.outputDirectory();
        std::filesystem::create_directories(outputDirectory, fsError);
        if (fsError) {
            return;
        }

        simulation.startXYZRecording(capture_utils::makeDatedCaptureOutputPath(outputDirectory, ".xyz").string());
    }

    std::unique_ptr<Generators::Region> makeRegion(const AppSignals::UI::GeneratorRegionSpec& spec) {
        using AppSignals::UI::GeneratorRegionKind;
        using namespace Generators;

        switch (spec.kind) {
        case GeneratorRegionKind::Box: {
            auto region = std::make_unique<Rectangle>();
            region->center = spec.center;
            region->boxSize = spec.boxSize;
            return region;
        }
        case GeneratorRegionKind::Sphere: {
            auto region = std::make_unique<Sphere>();
            region->center = spec.center;
            region->sphereRadius = spec.sphereRadius;
            return region;
        }
        case GeneratorRegionKind::Cylinder: {
            auto region = std::make_unique<Cylinder>();
            region->center = spec.center;
            region->baseRadius = spec.cylinderRadius;
            region->cylinderHeight = spec.cylinderHeight;
            return region;
        }
        case GeneratorRegionKind::Capsule: {
            auto region = std::make_unique<Capsule>();
            region->center = spec.center;
            region->capsuleRadius = spec.capsuleRadius;
            region->capsuleHeight = spec.capsuleHeight;
            return region;
        }
        case GeneratorRegionKind::Torus: {
            auto region = std::make_unique<Torus>();
            region->center = spec.center;
            region->majorRadius = spec.torusMajorRadius;
            region->tubeRadius = spec.torusTubeRadius;
            return region;
        }
        case GeneratorRegionKind::PolygonPrism: {
            auto region = std::make_unique<PolygonPrism>();
            region->polygonPoints = spec.polygonPoints;
            region->minZ = spec.prismMinZ;
            region->maxZ = spec.prismMaxZ;
            return region;
        }
        case GeneratorRegionKind::TrianglePyramid: {
            auto region = std::make_unique<TrianglePyramid>();
            region->center = spec.center;
            region->baseCircumradius = spec.pyramidBaseCircumradius;
            region->pyramidHeight = spec.pyramidHeight;
            return region;
        }
        case GeneratorRegionKind::TriangleBiPyramid: {
            auto region = std::make_unique<TriangleBiPyramid>();
            region->center = spec.center;
            region->baseCircumradius = spec.bipyramidBaseCircumradius;
            region->bipyramidHeight = spec.bipyramidHeight;
            return region;
        }
        }

        auto region = std::make_unique<Rectangle>();
        region->center = spec.center;
        region->boxSize = spec.boxSize;
        return region;
    }

    std::vector<Generators::Compose> makeComposition(const std::vector<AppSignals::UI::GeneratorComposeSpec>& composition) {
        std::vector<Generators::Compose> result;
        result.reserve(composition.size());
        for (const AppSignals::UI::GeneratorComposeSpec& entry : composition) {
            result.push_back({
                .species = entry.species,
                .fraction = entry.fraction,
            });
        }
        return result;
    }

    void appendSegment(std::vector<glm::vec3>& lines, glm::vec3 start, glm::vec3 end) {
        lines.push_back(start);
        lines.push_back(end);
    }

    glm::vec3 pointOnCircleXY(glm::vec3 center, float radius, float angle) {
        return glm::vec3(center.x + radius * std::cos(angle), center.y + radius * std::sin(angle), center.z);
    }

    glm::vec3 pointOnCircleXZ(glm::vec3 center, float radius, float angle) {
        return glm::vec3(center.x + radius * std::cos(angle), center.y, center.z + radius * std::sin(angle));
    }

    glm::vec3 pointOnCircleYZ(glm::vec3 center, float radius, float angle) {
        return glm::vec3(center.x, center.y + radius * std::cos(angle), center.z + radius * std::sin(angle));
    }

    template <typename PointFn>
    void appendClosedCurve(std::vector<glm::vec3>& lines, int segments, PointFn pointFn) {
        for (int i = 0; i < segments; ++i) {
            const float a0 = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
            const float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(segments);
            appendSegment(lines, pointFn(a0), pointFn(a1));
        }
    }

    std::vector<glm::vec3> buildPhantomLines(const AppSignals::UI::GeneratorRegionSpec& spec, const glm::vec3&, float inset = 0.0f) {
        using AppSignals::UI::GeneratorRegionKind;
        constexpr int kCircleSegments = 48;
        constexpr int kTorusMajorSegments = 48;
        constexpr int kTorusMinorSegments = 18;
        std::vector<glm::vec3> lines;
        const float clampedInset = std::max(inset, 0.0f);

        switch (spec.kind) {
        case GeneratorRegionKind::Box: {
            const glm::vec3 effectiveSize = glm::max(spec.boxSize - glm::vec3(2.0f * clampedInset), glm::vec3(0.0f));
            const glm::vec3 half = 0.5f * effectiveSize;
            const std::array<glm::vec3, 8> corners = {
                spec.center + glm::vec3(-half.x, -half.y, -half.z),
                spec.center + glm::vec3( half.x, -half.y, -half.z),
                spec.center + glm::vec3( half.x,  half.y, -half.z),
                spec.center + glm::vec3(-half.x,  half.y, -half.z),
                spec.center + glm::vec3(-half.x, -half.y,  half.z),
                spec.center + glm::vec3( half.x, -half.y,  half.z),
                spec.center + glm::vec3( half.x,  half.y,  half.z),
                spec.center + glm::vec3(-half.x,  half.y,  half.z),
            };
            constexpr std::array<std::pair<int, int>, 12> edges = {{
                {0, 1}, {1, 2}, {2, 3}, {3, 0},
                {4, 5}, {5, 6}, {6, 7}, {7, 4},
                {0, 4}, {1, 5}, {2, 6}, {3, 7},
            }};
            lines.reserve(edges.size() * 2);
            for (const auto& [a, b] : edges) {
                appendSegment(lines, corners[a], corners[b]);
            }
            break;
        }
        case GeneratorRegionKind::Sphere:
            lines.reserve(kCircleSegments * 3 * 2);
            appendClosedCurve(lines, kCircleSegments,
                              [&](float angle) { return pointOnCircleXY(spec.center, std::max(0.0f, spec.sphereRadius - clampedInset), angle); });
            appendClosedCurve(lines, kCircleSegments,
                              [&](float angle) { return pointOnCircleXZ(spec.center, std::max(0.0f, spec.sphereRadius - clampedInset), angle); });
            appendClosedCurve(lines, kCircleSegments,
                              [&](float angle) { return pointOnCircleYZ(spec.center, std::max(0.0f, spec.sphereRadius - clampedInset), angle); });
            break;
        case GeneratorRegionKind::Cylinder: {
            const float effectiveRadius = std::max(0.0f, spec.cylinderRadius - clampedInset);
            const float effectiveHeight = std::max(0.0f, spec.cylinderHeight - 2.0f * clampedInset);
            const float halfHeight = 0.5f * effectiveHeight;
            const glm::vec3 bottomCenter = spec.center + glm::vec3(0.0f, 0.0f, -halfHeight);
            const glm::vec3 topCenter = spec.center + glm::vec3(0.0f, 0.0f, halfHeight);
            appendClosedCurve(lines, kCircleSegments,
                              [&](float angle) { return pointOnCircleXY(bottomCenter, effectiveRadius, angle); });
            appendClosedCurve(lines, kCircleSegments,
                              [&](float angle) { return pointOnCircleXY(topCenter, effectiveRadius, angle); });
            for (int i = 0; i < 4; ++i) {
                const float angle = glm::half_pi<float>() * static_cast<float>(i);
                appendSegment(lines, pointOnCircleXY(bottomCenter, effectiveRadius, angle),
                              pointOnCircleXY(topCenter, effectiveRadius, angle));
            }
            break;
        }
        case GeneratorRegionKind::Capsule: {
            const float halfHeight = 0.5f * spec.capsuleHeight;
            const glm::vec3 bottomCenter = spec.center + glm::vec3(0.0f, 0.0f, -halfHeight);
            const glm::vec3 topCenter = spec.center + glm::vec3(0.0f, 0.0f, halfHeight);
            appendClosedCurve(lines, kCircleSegments,
                              [&](float angle) { return pointOnCircleXY(bottomCenter, spec.capsuleRadius, angle); });
            appendClosedCurve(lines, kCircleSegments,
                              [&](float angle) { return pointOnCircleXY(topCenter, spec.capsuleRadius, angle); });
            for (int i = 0; i < 4; ++i) {
                const float angle = glm::half_pi<float>() * static_cast<float>(i);
                appendSegment(lines, pointOnCircleXY(bottomCenter, spec.capsuleRadius, angle),
                              pointOnCircleXY(topCenter, spec.capsuleRadius, angle));
            }
            for (int i = 0; i < kCircleSegments; ++i) {
                const float a0 = glm::pi<float>() * static_cast<float>(i) / static_cast<float>(kCircleSegments);
                const float a1 = glm::pi<float>() * static_cast<float>(i + 1) / static_cast<float>(kCircleSegments);
                appendSegment(lines, pointOnCircleXZ(topCenter, spec.capsuleRadius, a0),
                              pointOnCircleXZ(topCenter, spec.capsuleRadius, a1));
                appendSegment(lines, pointOnCircleXZ(bottomCenter, spec.capsuleRadius, a0 + glm::pi<float>()),
                              pointOnCircleXZ(bottomCenter, spec.capsuleRadius, a1 + glm::pi<float>()));
                appendSegment(lines, pointOnCircleYZ(topCenter, spec.capsuleRadius, a0),
                              pointOnCircleYZ(topCenter, spec.capsuleRadius, a1));
                appendSegment(lines, pointOnCircleYZ(bottomCenter, spec.capsuleRadius, a0 + glm::pi<float>()),
                              pointOnCircleYZ(bottomCenter, spec.capsuleRadius, a1 + glm::pi<float>()));
            }
            break;
        }
        case GeneratorRegionKind::Torus:
            for (int minorIndex = 0; minorIndex < 4; ++minorIndex) {
                const float minorAngle = glm::half_pi<float>() * static_cast<float>(minorIndex);
                appendClosedCurve(lines, kTorusMajorSegments, [&](float majorAngle) {
                    const float effectiveTubeRadius = std::max(0.0f, spec.torusTubeRadius - clampedInset);
                    const float ringRadius = spec.torusMajorRadius + effectiveTubeRadius * std::cos(minorAngle);
                    return glm::vec3(
                        spec.center.x + ringRadius * std::cos(majorAngle),
                        spec.center.y + ringRadius * std::sin(majorAngle),
                        spec.center.z + effectiveTubeRadius * std::sin(minorAngle)
                    );
                });
            }
            for (int majorIndex = 0; majorIndex < 8; ++majorIndex) {
                const float majorAngle = glm::two_pi<float>() * static_cast<float>(majorIndex) / 8.0f;
                appendClosedCurve(lines, kTorusMinorSegments, [&](float minorAngle) {
                    const float effectiveTubeRadius = std::max(0.0f, spec.torusTubeRadius - clampedInset);
                    const float radial = spec.torusMajorRadius + effectiveTubeRadius * std::cos(minorAngle);
                    return glm::vec3(
                        spec.center.x + radial * std::cos(majorAngle),
                        spec.center.y + radial * std::sin(majorAngle),
                        spec.center.z + effectiveTubeRadius * std::sin(minorAngle)
                    );
                });
            }
            break;
        case GeneratorRegionKind::PolygonPrism: {
            if (spec.polygonPoints.size() < 2) {
                break;
            }
            const float minZ = spec.prismMinZ + clampedInset;
            const float maxZ = spec.prismMaxZ - clampedInset;
            if (minZ > maxZ) {
                break;
            }
            for (size_t i = 0; i < spec.polygonPoints.size(); ++i) {
                const glm::vec2& a = spec.polygonPoints[i];
                const glm::vec2& b = spec.polygonPoints[(i + 1) % spec.polygonPoints.size()];
                appendSegment(lines, glm::vec3(a.x, a.y, minZ), glm::vec3(b.x, b.y, minZ));
                appendSegment(lines, glm::vec3(a.x, a.y, maxZ), glm::vec3(b.x, b.y, maxZ));
                appendSegment(lines, glm::vec3(a.x, a.y, minZ), glm::vec3(a.x, a.y, maxZ));
            }
            break;
        }
        case GeneratorRegionKind::TrianglePyramid: {
            const float halfHeight = 0.5f * spec.pyramidHeight;
            const float r = spec.pyramidBaseCircumradius;
            const float sqrt3 = std::sqrt(3.0f);
            const glm::vec3 baseCenter = spec.center + glm::vec3(0.0f, 0.0f, -halfHeight);
            const glm::vec3 apex = spec.center + glm::vec3(0.0f, 0.0f, halfHeight);
            const std::array<glm::vec3, 3> base = {
                baseCenter + glm::vec3(0.0f, r, 0.0f),
                baseCenter + glm::vec3(-0.5f * sqrt3 * r, -0.5f * r, 0.0f),
                baseCenter + glm::vec3(0.5f * sqrt3 * r, -0.5f * r, 0.0f),
            };
            appendSegment(lines, base[0], base[1]);
            appendSegment(lines, base[1], base[2]);
            appendSegment(lines, base[2], base[0]);
            for (const glm::vec3& point : base) {
                appendSegment(lines, point, apex);
            }
            break;
        }
        case GeneratorRegionKind::TriangleBiPyramid: {
            const float halfHeight = 0.5f * spec.bipyramidHeight;
            const float r = spec.bipyramidBaseCircumradius;
            const float sqrt3 = std::sqrt(3.0f);
            const glm::vec3 topApex = spec.center + glm::vec3(0.0f, 0.0f, halfHeight);
            const glm::vec3 bottomApex = spec.center + glm::vec3(0.0f, 0.0f, -halfHeight);
            const std::array<glm::vec3, 3> base = {
                spec.center + glm::vec3(0.0f, r, 0.0f),
                spec.center + glm::vec3(-0.5f * sqrt3 * r, -0.5f * r, 0.0f),
                spec.center + glm::vec3(0.5f * sqrt3 * r, -0.5f * r, 0.0f),
            };
            appendSegment(lines, base[0], base[1]);
            appendSegment(lines, base[1], base[2]);
            appendSegment(lines, base[2], base[0]);
            for (const glm::vec3& point : base) {
                appendSegment(lines, point, topApex);
                appendSegment(lines, point, bottomApex);
            }
            break;
        }
        }

        return lines;
    }
}

namespace AppActions {
    void Handler::applySmoothResizeStep(Lattice::Simulation& simulation) {
        if (!smoothResizeActive_) {
            return;
        }

        const float maxStep = smoothResizeMaxSpeed_ * std::max(simulation.getDt(), 0.0f);
        if (maxStep <= 0.0f) {
            return;
        }

        const glm::vec3 nextSize = moveTowards(simulation.world().getWorldSize(), smoothResizeTarget_, maxStep);
        applyResizeBox(simulation, nextSize);
        if (nextSize == smoothResizeTarget_) {
            smoothResizeActive_ = false;
        }
    }

    void Handler::trackIOPanel(CaptureController& captureController, UiState& uiState, Lattice::Simulation& simulation, SceneViewport& renderer) {
        track(AppSignals::UI::SaveSimulation.connect(
            [&](std::string_view path) { AppStateIO::save(captureController, uiState.scenePreviewRect, simulation, renderer.renderer(), path); }));
        track(AppSignals::UI::LoadSimulation.connect([&](std::string_view path) {
            AppStateIO::load(simulation, renderer.renderer(), path);
            renderer.syncScene(simulation);
            ToolsManager::resetInteractionState();
        }));
        track(AppSignals::UI::ResizeBox.connect([&](const glm::vec3& newSize) {
            smoothResizeActive_ = false;
            applyResizeBox(simulation, newSize);
        }));
        track(AppSignals::UI::SmoothResizeBox.connect([&](const glm::vec3& newSize, float maxSpeed) {
            smoothResizeTarget_ = clampBoxSize(newSize);
            smoothResizeMaxSpeed_ = std::max(maxSpeed, 0.0f);
            smoothResizeActive_ = true;
            if (smoothResizeMaxSpeed_ <= 0.0f) {
                applyResizeBox(simulation, smoothResizeTarget_);
                smoothResizeActive_ = false;
            }
        }));
        track(AppSignals::UI::ClearSimulation.connect([&]() {
            simulation.clear();
            ToolsManager::resetInteractionState();
            smoothResizeActive_ = false;
        }));
        track(AppSignals::UI::CreateTriangularBipyramidCrystal.connect([&](int axisCount, AtomData::Type atomType) {
            simulation.clear();
            ToolsManager::resetInteractionState();
            Generators::triangularBipyramidCrystal(simulation, axisCount, atomType);
        }));
        track(AppSignals::UI::CreateRandomFill.connect([&](const AppSignals::UI::RandomFillRequest& request) {
            if (request.options.mode == Generators::SpawnMode::Reset) {
                simulation.clear();
            }
            ToolsManager::resetInteractionState();
            const std::unique_ptr<Generators::Region> region = makeRegion(request.region);
            Generators::randomFill(simulation, *region, makeComposition(request.composition), request.options);
            renderer.syncScene(simulation);
        }));
        track(AppSignals::UI::CreateLatticeFill.connect([&](const AppSignals::UI::LatticeFillRequest& request) {
            if (request.options.mode == Generators::SpawnMode::Reset) {
                simulation.clear();
            }
            ToolsManager::resetInteractionState();
            const std::unique_ptr<Generators::Region> region = makeRegion(request.region);
            Generators::latticeFill(simulation, *region, makeComposition(request.composition), request.options);
            renderer.syncScene(simulation);
        }));
        track(AppSignals::UI::SetGeneratorPhantom.connect([&](const AppSignals::UI::GeneratorRegionSpec& region) {
            if (renderer.renderer().getRenderDataCount() <= simulation.activeWorldId()) {
                return;
            }
            RenderData& renderData = renderer.renderer().getRenderData(simulation.activeWorldId());
            renderData.phantomLines = buildPhantomLines(region, simulation.world().getWorldSize());
            renderData.drawPhantom = !renderData.phantomLines.empty();
        }));
        track(AppSignals::UI::ClearGeneratorPhantom.connect([&]() {
            if (renderer.renderer().getRenderDataCount() <= simulation.activeWorldId()) {
                return;
            }
            RenderData& renderData = renderer.renderer().getRenderData(simulation.activeWorldId());
            renderData.phantomLines.clear();
            renderData.drawPhantom = false;
        }));
        track(AppSignals::Capture::ToggleXYZRecording.connect([&]() { toggleXYZRecording(captureController, simulation); }));
    }

    void Handler::trackToolsPanel(Lattice::Simulation& simulation, SceneViewport& renderer) {
        track(AppSignals::UI::SetRender.connect([&](RendererType type) {
            const SceneViewport::RendererType rendererType =
                (type == RendererType::Renderer2D) ? SceneViewport::RendererType::Renderer2D : SceneViewport::RendererType::Renderer3D;
            if (renderer.setRendererType(rendererType, simulation)) {
                ToolsManager::resetInteractionState();
            }
        }));

        track(AppSignals::UI::SetCameraMode.connect([&](Camera::Mode mode) { renderer.renderer().camera.setMode(mode); }));
    }

    void Handler::trackSettingsPanel(GLFWwindow* window) {
        track(AppSignals::UI::ExitApplication.connect([window]() { glfwSetWindowShouldClose(window, GLFW_TRUE); }));
    }

    void Handler::trackKeyboard(Lattice::Simulation& simulation) {
        track(AppSignals::Keyboard::StepPhysics.connect([&]() {
            applySmoothResizeStep(simulation);
            simulation.update();
        }));
    }

    void Handler::trackSimControlPanel(Lattice::Simulation& simulation) {
        track(AppSignals::UI::StepPhysics.connect([&]() {
            applySmoothResizeStep(simulation);
            simulation.update();
        }));
    }

    Handler::Handler(GLFWwindow* window, CaptureController& captureController, Lattice::Simulation& simulation, SceneViewport& renderer, UiState& uiState) {
        trackIOPanel(captureController, uiState, simulation, renderer);
        trackToolsPanel(simulation, renderer);
        trackSettingsPanel(window);
        trackSimControlPanel(simulation);
        trackKeyboard(simulation);
    }

    void Handler::updateSimulationStep(Lattice::Simulation& simulation) {
        applySmoothResizeStep(simulation);
    }
}
