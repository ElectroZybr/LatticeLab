<p align="center">
  <img src="demo/logo.png" alt="LatticeLab logo" width="720">
</p>

<p align="center">
  Interactive matter simulation using <b>LATTICE</b>.
</p>

## About

LatticeLab is a project built around the `LATTICE` molecular dynamics engine for interactive simulation and exploration of atomic behavior.

The idea is simple:

> define how atoms interact and observe the resulting behavior

Without any animations made in advance.  
Only potentials and forces between them.

But that is already enough for the system to behave very much like real matter.

---

## What You Can Observe

- self-formation of crystals
- particle diffusion
- lattice oscillations, waves, and phonon-like vibration
- emergence of grains and domain boundaries
- packing defects and local lattice distortions
- nucleation of inhomogeneities
- waves, phonons, and interference
- local ordering and structural breakdown
- behavior resembling solids, liquids, and transitional states
- relaxation after collisions, compression, or parameter changes

---

## What Is Planned

- molecular modeling
- chemical reactions
- metals and alloys
- charged particles and ions
- [x] electrostatic interaction (Coulomb)
- electricity and conductivity
- simple models of nuclear interactions
- radioactive decay
- energy transitions
- biochemistry
- proteins and folding-like behavior
- DNA and other chain-like molecular systems
- [project tree and module map](https://latticelab.dev/tree.html)

---

## Why?

Physics is often presented as equations first.

Here, you can watch complex behavior emerge from simple interaction rules:

- for educational purposes
- for experimenting
- to understand how the world around us works

---

## Documentation

- [English documentation entry](Docs/en/Main.md)
- [Russian documentation entry](Docs/ru/Main.md)

If you want to start with project internals, the best first articles are:

- [Architecture](Docs/en/articles/Architecture.md)
- [Build Guide](Docs/en/articles/Build.md)
- [LATTICE Engine](Docs/en/articles/Lattice.md)
- [Rendering](Docs/en/articles/Rendering.md)

---

## Build

The project uses `CMakePresets`.

Typical local build:

```bash
cmake --preset debug
cmake --build --preset debug
```

Release build:

```bash
cmake --preset release
cmake --build --preset release
```

---

## Repository Structure

- `App/` - application layer, UI, viewport, input, and integration code
- `Lattice/` - simulation engine, physical model, and core data structures
- `Rendering/` - rendering backend, GPU pipelines, camera, and draw logic
- `Docs/` - project documentation
- `assets/` - runtime assets
- `demo/` - demo scenes and media

---

## Links

#### [Official site](https://latticelab.dev)
#### [YouTube channel](https://www.youtube.com/@ElectroChajnik) (Russian)
#### [Support the project!](https://latticelab.dev/donate.html)
