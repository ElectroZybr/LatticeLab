#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec3.hpp>
#include "Lattice/Engine/physics/Atom/AtomData.h"
#include "Lattice/Generators/Gas.h"
#include "Lattice/Generators/LatticeFill.hpp"
#include "Lattice/Generators/RandomFill.hpp"
#include "GUI/interface/panels/tools/ToolsPanel.h"
#include "Rendering/camera/Camera.h"
#include "App/Signals.h"

namespace AppSignals {
    namespace UI {
        enum class GeneratorRegionKind : uint8_t {
            Box,
            Sphere,
            Cylinder,
            Capsule,
            Torus,
            TrianglePyramid,
            TriangleBiPyramid,
        };

        struct GeneratorComposeSpec {
            std::string species;
            float fraction = 1.0f;
        };

        struct GeneratorRegionSpec {
            GeneratorRegionKind kind = GeneratorRegionKind::Box;
            glm::vec3 center = glm::vec3(60.0f, 60.0f, 60.0f);
            glm::vec3 boxSize = glm::vec3(80.0f, 80.0f, 80.0f);
            float sphereRadius = 20.0f;
            float cylinderRadius = 20.0f;
            float cylinderHeight = 40.0f;
            float capsuleRadius = 20.0f;
            float capsuleHeight = 40.0f;
            float torusMajorRadius = 20.0f;
            float torusTubeRadius = 8.0f;
            float pyramidBaseCircumradius = 20.0f;
            float pyramidHeight = 40.0f;
            float bipyramidBaseCircumradius = 20.0f;
            float bipyramidHeight = 40.0f;
        };

        struct RandomFillRequest {
            GeneratorRegionSpec region;
            std::vector<GeneratorComposeSpec> composition;
            Generators::RandomFillOptions options;
        };

        struct LatticeFillRequest {
            GeneratorRegionSpec region;
            std::vector<GeneratorComposeSpec> composition;
            Generators::LatticeFillOptions options;
        };

        inline Signals::Signal<void(const glm::vec3& newSize)> ResizeBox;
        inline Signals::Signal<void(const glm::vec3& newSize, float maxSpeed)> SmoothResizeBox;

        inline Signals::Signal<void()> ClearSimulation;
        inline Signals::Signal<void(int atomCount, AtomData::Type atomType, bool is3D, float density)> CreateGas;
        inline Signals::Signal<void(int atomCount, std::vector<Generators::AtomTypeSpec> atomSpecs, bool is3D, float density)> CreateMixedGas;
        inline Signals::Signal<void(glm::ivec3 axisCounts, AtomData::Type atomType, bool is3D)> CreateMassive;
        inline Signals::Signal<void(glm::ivec3 axisCounts, AtomData::Type atomType)> CreateHexLattice;
        inline Signals::Signal<void(int axisCount, AtomData::Type atomType)> CreateTriangularBipyramidCrystal;
        inline Signals::Signal<void(const RandomFillRequest& request)> CreateRandomFill;
        inline Signals::Signal<void(const LatticeFillRequest& request)> CreateLatticeFill;

        inline Signals::Signal<void(RendererType type)> SetRender;
        inline Signals::Signal<void(Camera::Mode mode)> SetCameraMode;

        inline Signals::Signal<void(std::string_view path)> SaveSimulation;
        inline Signals::Signal<void(std::string_view path)> LoadSimulation;
        inline Signals::Signal<void()> ExitApplication;

        inline Signals::Signal<void()> StepPhysics;
    }

    namespace Capture {
        inline Signals::Signal<void(std::string_view path)> SetOutputDirectory;
        inline Signals::Signal<void()> ToggleRecording;
        inline Signals::Signal<void()> ToggleXYZRecording;
    }

    namespace Keyboard {
        inline Signals::Signal<void()> StepPhysics;
    }
}
