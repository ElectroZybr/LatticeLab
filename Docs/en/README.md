> Language: EN
> RU status: source article available

# LatticeLab Documentation

This section is the English entry point into the project documentation: what this repository is, how it is structured, how to build it, and where to look in the codebase.

If you need the main project overview, see [README](../../README.md).
If you need an overview of the project architecture and the role of the `LATTICE` engine, see [Architecture](./articles/Architecture.md).

## Overview

LatticeLab is a desktop application for interactive matter simulation built on top of the `LATTICE` engine.

The repository is split into several major parts:

- `App/` - application layer, windowing, viewport, UI, and glue code
- `Lattice/` - physical model, scene data, and simulation logic
- `Rendering/` - rendering, GPU pipelines, draw layers, rendering benchmarks, and tests
- `Docs/` - project documentation
- `assets/` - application assets
- `demo/` - demo scenes and media

## Navigation

[README.md](./README.md)
├── [Architecture](./articles/Architecture.md)
├── [App](./articles/App.md)
├── [Build](./articles/Build.md)
├── [Data Flow](./articles/DataFlow.md)
├── [GUI](./articles/GUI.md)
├── [Glossary](./articles/Glossary.md)
├── [Localization](./articles/Localization.md)
├── [Lattice](./articles/Lattice.md)
├── [Rendering](./articles/Rendering.md)
├── [Tests](./articles/Tests.md)
├── [WGPU](./articles/WGPU.md)
├── [CLI](./articles/CLI.md)
└── [Benchmarks](./articles/Benchmarks.md)

The English article set is currently a skeleton with working links. The Russian tree in `Docs/ru/articles/` remains the source set for full content.

## Code

If you want to jump straight into the main code entry points:

- [main.cpp](../../main.cpp)
- [App/Application.cpp](../../App/Application.cpp)
- [App/viewport/SceneViewport.cpp](../../App/viewport/SceneViewport.cpp)
- [App/viewport/SimulationSceneSource.h](../../App/viewport/SimulationSceneSource.h)
- [Lattice/Engine/Simulation.h](../../Lattice/Engine/Simulation.h)
- [Lattice/Engine/World.h](../../Lattice/Engine/World.h)
- [Rendering/Renderer.h](../../Rendering/Renderer.h)
- [Rendering/Renderer.cpp](../../Rendering/Renderer.cpp)
- [Rendering/RenderData.h](../../Rendering/RenderData.h)
- [Rendering/backend/WGPUContext.h](../../Rendering/backend/WGPUContext.h)

## Build

The full build guide is currently available in the Russian documentation: [Build Guide](./articles/Build.md).

In short:

- the project uses `CMakePresets`
- normal local builds usually go through `debug` or `release`
- benchmarks use a separate `bench` preset

```bash
cmake --preset debug
cmake --build --preset debug
```
