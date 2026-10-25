<p align="center">
  <img src="assets/logo.png" alt="LatticeLab logo" width="720">
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

Documentation is in Russian for now: [docs/ru/README.md](docs/ru/README.md).

- [Architecture](docs/ru/articles/Architecture.md) — how the project is built on the Lattice runtime
- [Build and run](docs/ru/Build.md)
- [Development rules](docs/ru/Rules.md) and [antipatterns](docs/ru/Antipatterns.md)
- [Plans](docs/ru/plans/README.md)

---

## Build

The project uses [xmake](https://xmake.io) and a C++23 compiler. The Lattice runtime is expected next to
this repository (`../Lattice`, see the `lattice_source` option); third-party dependencies are installed by xmake.

```bash
xmake f -y
xmake build -y
./LatticeLab
```

What starts is configured in `lattice.toml`. Details: [Build.md](docs/ru/Build.md).

---

## Repository Structure

- `packages/` - Lattice packages: each composes its own branch of the runtime tree (`compose.cpp`)
- `libs/` - plain libraries: models, algorithms and their CPU/GPU backends, no runtime
- `docs/` - documentation
- `config/` - runtime defaults
- `user/` - local user data such as saved configuration, not tracked in git
- `assets/` - resources
- `app/`, `legacy/` - the old application, being moved into packages

---

## Links

#### [Official site](https://latticelab.dev)
#### [YouTube channel](https://www.youtube.com/@ElectroChajnik) (Russian)
#### [Support the project!](https://latticelab.dev/donate.html)
