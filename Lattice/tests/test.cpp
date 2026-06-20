#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

#include <glm/vec3.hpp>

#include "Engine/Simulation.h"
#include "Engine/io/MoleculePdb.h"
#include "Engine/NeighborSearch/BarnesHut/Octree.h"
#include "Engine/NeighborSearch/SpatialGrid.h"
#include "Engine/physics/Atom/AtomSort.h"
#include "Engine/physics/Atom/AtomStorage.h"
#include "Lattice/Plugins/ClassicMD/ForceFields/CoulombForceField.h"
#include "Lattice/Plugins/ClassicMD/ClassicMDPlugin.h"
#include "Scripting/LuaState.h"

struct OctreeTestSupport {
    static float charge(const OctreeNode& node) { return node.charge; }
    static void setCharge(OctreeNode& node, float value) { node.charge = value; }

    static const glm::vec3& center(const OctreeNode& node) { return node.center; }
    static void setCenter(OctreeNode& node, const glm::vec3& value) { node.center = value; }

    static const glm::vec3& dipole(const OctreeNode& node) { return node.dipoleMoment; }
    static void setDipole(OctreeNode& node, const glm::vec3& value) { node.dipoleMoment = value; }

    static size_t atomCount(const OctreeNode& node) { return node.atomCount; }
    static void setAtomCount(OctreeNode& node, size_t count) { node.atomCount = count; }

    static float size(const OctreeNode& node) { return node.size; }
    static void setSize(OctreeNode& node, float value) { node.size = value; }

    static size_t firstAtom(const OctreeNode& node) { return node.firstAtom; }
    static void setFirstAtom(OctreeNode& node, size_t value) { node.firstAtom = value; }

    static const OctreeNode* child(const OctreeNode& node, int index) { return node.children[index].get(); }
    static OctreeNode* child(OctreeNode& node, int index) { return node.children[index].get(); }
    static void setChild(OctreeNode& node, int index, std::unique_ptr<OctreeNode> childNode) { node.children[index] = std::move(childNode); }
};

static bool isClose(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) <= eps;
}

static bool isClose(const glm::vec3& a, const glm::vec3& b, float eps = 1e-4f) {
    return isClose(a.x, b.x, eps) && isClose(a.y, b.y, eps) && isClose(a.z, b.z, eps);
}

static void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "TEST FAILED: " << message << std::endl;
        std::terminate();
    }
}

static void testOctreeBuildChargeAndChildren() {
    const size_t count = 3;
    std::vector<float> x{1.0f, 5.0f, 1.0f};
    std::vector<float> y{1.0f, 1.0f, 5.0f};
    std::vector<float> z{1.0f, 1.0f, 1.0f};
    std::vector<float> vx(count, 0.0f);
    std::vector<float> vy(count, 0.0f);
    std::vector<float> vz(count, 0.0f);
    std::vector<AtomData::Type> types(count, AtomData::Type::H);
    std::vector<float> charge{1.0f, -2.0f, 3.0f};

    AtomStorage atoms;
    atoms.init(count, count, x, y, z, vx, vy, vz, types, charge);

    SpatialGrid grid(glm::vec3(8.0f), 4.0f);
    grid.rebuild(atoms.xDataSpan(), atoms.yDataSpan(), atoms.zDataSpan());

    AtomSort sorter;
    sorter.mortonOrder(atoms, grid);

    OctreeNode root;
    root.build(atoms, grid);

    expect(isClose(OctreeTestSupport::charge(root), 2.0f), "Octree root charge should equal total atom charge");
    expect(OctreeTestSupport::atomCount(root) == count, "Octree root atom count should equal number of atoms");

    float childChargeSum = 0.0f;
    int nonEmptyChildCount = 0;
    for (int i = 0; i < 8; ++i) {
        const OctreeNode* child = OctreeTestSupport::child(root, i);
        if (child != nullptr) {
            ++nonEmptyChildCount;
            childChargeSum += OctreeTestSupport::charge(*child);
            expect(OctreeTestSupport::atomCount(*child) >= 1, "Non-empty child node must contain at least one atom");
        }
    }

    expect(nonEmptyChildCount == 1, "Only one top-level octant should contain all atoms for this configuration");
    expect(isClose(childChargeSum, 2.0f), "Sum of child charges should equal root charge");

    const OctreeNode* primaryChild = OctreeTestSupport::child(root, 0);
    expect(primaryChild != nullptr, "Primary top-level child must exist");

    float grandchildChargeSum = 0.0f;
    int nonEmptyGrandchildCount = 0;
    for (int i = 0; i < 8; ++i) {
        const OctreeNode* grandchild = OctreeTestSupport::child(*primaryChild, i);
        if (grandchild != nullptr) {
            ++nonEmptyGrandchildCount;
            grandchildChargeSum += OctreeTestSupport::charge(*grandchild);
            expect(OctreeTestSupport::atomCount(*grandchild) >= 1, "Non-empty grandchild node must contain at least one atom");
        }
    }

    expect(nonEmptyGrandchildCount == 3, "Three second-level octants should contain atoms for this configuration");
    expect(isClose(grandchildChargeSum, 2.0f), "Sum of grandchild charges should equal root charge");
}

static void testCoulombFarFieldApproximation() {
    const size_t count = 2;
    std::vector<float> x{0.0f, 100.0f};
    std::vector<float> y{0.0f, 0.0f};
    std::vector<float> z{0.0f, 0.0f};
    std::vector<float> vx(count, 0.0f);
    std::vector<float> vy(count, 0.0f);
    std::vector<float> vz(count, 0.0f);
    std::vector<AtomData::Type> types(count, AtomData::Type::H);
    std::vector<float> charge{1.0f, 1.0f};

    AtomStorage atoms;
    atoms.init(count, count, x, y, z, vx, vy, vz, types, charge);

    OctreeNode root;
    OctreeTestSupport::setSize(root, 1.0f);
    OctreeTestSupport::setCenter(root, glm::vec3(100.0f, 0.0f, 0.0f));
    OctreeTestSupport::setCharge(root, 1.0f);
    OctreeTestSupport::setDipole(root, glm::vec3(100.0f, 0.0f, 0.0f));
    OctreeTestSupport::setFirstAtom(root, 1);
    OctreeTestSupport::setAtomCount(root, 1);
    OctreeTestSupport::setChild(root, 0, std::make_unique<OctreeNode>(OctreeTestSupport::center(root)));
    OctreeNode* cluster = OctreeTestSupport::child(root, 0);
    OctreeTestSupport::setCharge(*cluster, 1.0f);
    OctreeTestSupport::setCenter(*cluster, OctreeTestSupport::center(root));
    OctreeTestSupport::setDipole(*cluster, OctreeTestSupport::dipole(root));
    OctreeTestSupport::setFirstAtom(*cluster, 1);
    OctreeTestSupport::setAtomCount(*cluster, 1);

    CoulombForceField field;
    float fx = 0.0f;
    float fy = 0.0f;
    float fz = 0.0f;
    float potentialEnergy = 0.0f;

    field.computeForce(atoms, 0, root, 0.7f, fx, fy, fz, potentialEnergy);

    const float d = 100.0f;
    const float d2 = d * d;
    const float invR = 1.0f / d;
    const float qqScale = CoulombForceField::kCoulombEvAngstrom * 1.0f * 1.0f;
    const float expectedForceX = -100.0f * qqScale * invR / d2;
    const float expectedPotential = 0.5f * qqScale * invR;

    expect(isClose(fx, expectedForceX, 1e-5f), "Far-field force should match monopole approximation in X direction");
    expect(isClose(fy, 0.0f, 1e-6f), "Far-field force should be zero in Y direction");
    expect(isClose(fz, 0.0f, 1e-6f), "Far-field force should be zero in Z direction");
    expect(isClose(potentialEnergy, expectedPotential, 1e-5f), "Far-field potential should match monopole approximation");
}

static float bondDistance(const Lattice::Simulation& simulation, const Bond& bond) {
    const glm::vec3 a = simulation.atoms().pos(bond.aIndex);
    const glm::vec3 b = simulation.atoms().pos(bond.bIndex);
    return glm::length(a - b);
}

static void testSpawnWaterMoleculeCreatesLocalAtoms() {
    Lattice::Simulation simulation;
    simulation.createWorld(glm::vec3(20.0f, 20.0f, 20.0f));

    const std::filesystem::path waterPath = std::filesystem::path("Mods") / "Base" / "Molecules" / "h2o.pdb";
    expect(simulation.loadMoleculeTemplate("h2o", waterPath), "Water template should load");

    expect(simulation.spawnMolecule("h2o", glm::vec3(10.0f, 10.0f, 10.0f), glm::mat3(1.0f), false), "Direct water spawn should succeed");
    expect(std::ranges::distance(simulation.bonds()) == 2, "Direct water spawn should create two bonds");

    for (const Bond& bond : simulation.bonds()) {
        expect(bondDistance(simulation, bond) < 2.0f, "Directly spawned water bond should stay short");
    }
}

static void testSpawnNitrogenMoleculeCreatesStableBond() {
    Lattice::Simulation simulation;
    simulation.createWorld(glm::vec3(20.0f, 20.0f, 20.0f));

    const std::filesystem::path nitrogenPath = std::filesystem::path("Mods") / "Base" / "Molecules" / "n2.pdb";
    expect(simulation.loadMoleculeTemplate("n2", nitrogenPath), "Nitrogen template should load");

    expect(simulation.spawnMolecule("n2", glm::vec3(10.0f, 10.0f, 10.0f), glm::mat3(1.0f), false), "Direct nitrogen spawn should succeed");
    expect(std::ranges::distance(simulation.bonds()) == 1, "Direct nitrogen spawn should create one bond");

    for (const Bond& bond : simulation.bonds()) {
        expect(bondDistance(simulation, bond) < 1.5f, "Directly spawned nitrogen bond should stay short");
    }
}

static void testSpawnAdditionalDiatomicMoleculesCreateStableBond() {
    constexpr std::array<std::string_view, 7> kMolecules = {
        "cl2",
        "f2",
        "br2",
        "hf",
        "hcl",
        "co",
        "no",
    };

    for (std::string_view moleculeName : kMolecules) {
        Lattice::Simulation simulation;
        simulation.createWorld(glm::vec3(20.0f, 20.0f, 20.0f));

        const std::filesystem::path moleculePath = std::filesystem::path("Mods") / "Base" / "Molecules" / (std::string(moleculeName) + ".pdb");
        expect(simulation.loadMoleculeTemplate(std::string(moleculeName), moleculePath), "Molecule template should load");

        expect(simulation.spawnMolecule(std::string(moleculeName), glm::vec3(10.0f, 10.0f, 10.0f), glm::mat3(1.0f), false),
               "Direct diatomic spawn should succeed");
        expect(std::ranges::distance(simulation.bonds()) == 1, "Direct diatomic spawn should create one bond");

        for (const Bond& bond : simulation.bonds()) {
            expect(bondDistance(simulation, bond) < 3.0f, "Directly spawned diatomic bond should stay short");
        }
    }
}

static void testCheckedMoleculeSpawnRejectsBlockedPoint() {
    Lattice::Simulation simulation;
    simulation.createWorld(glm::vec3(20.0f, 20.0f, 20.0f));

    const std::filesystem::path waterPath = std::filesystem::path("Mods") / "Base" / "Molecules" / "h2o.pdb";
    expect(simulation.loadMoleculeTemplate("h2o", waterPath), "Water template should load");

    simulation.createAtom(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f), AtomData::Type::O, false);
    const size_t atomCountBefore = simulation.atoms().size();

    expect(!simulation.canSpawnMolecule("h2o", glm::vec3(10.0f, 10.0f, 10.0f), glm::mat3(1.0f)),
           "Water molecule should not fit into an occupied spawn point");
    expect(!simulation.spawnMoleculeChecked("h2o", glm::vec3(10.0f, 10.0f, 10.0f), glm::mat3(1.0f), false),
           "Checked water spawn should fail for an occupied point");
    expect(simulation.atoms().size() == atomCountBefore, "Failed checked spawn should not add atoms");
}

static void testWaterMoleculeBondsStayLocalAfterNeighborSort() {
    Lattice::Simulation simulation;
    simulation.createWorld(glm::vec3(40.0f, 40.0f, 40.0f));

    const std::filesystem::path waterPath = std::filesystem::path("Mods") / "Base" / "Molecules" / "h2o.pdb";
    expect(simulation.loadMoleculeTemplate("h2o", waterPath), "Water template should load");

    Lattice::SpawnOptions options;
    options.min = glm::vec3(0.0f);
    options.max = glm::vec3(40.0f, 40.0f, 40.0f);
    options.margin = 6.0f;
    options.minDistance = 4.0f;
    options.maxAttempts = 128;
    options.randomRotation = true;

    const int moleculeCount = 20;
    for (int i = 0; i < moleculeCount; ++i) {
        expect(simulation.randomSpawn("h2o", options), "Each spawned water molecule should succeed");
    }

    expect(std::ranges::distance(simulation.bonds()) == moleculeCount * 2, "Each water molecule should contribute exactly two bonds");

    for (const Bond& bond : simulation.bonds()) {
        expect(bondDistance(simulation, bond) < 2.0f, "Freshly spawned water bond should stay short");
    }

    simulation.neighborList().rebuildPipeline(simulation.atoms(), simulation.world(), 0);

    for (const Bond& bond : simulation.bonds()) {
        expect(bondDistance(simulation, bond) < 2.0f, "Water bond should stay short after atom sorting/remap");
    }
}

static void testLuaSceneObjectLoadsMoleculesDirectory() {
    Lattice::Simulation simulation;
    simulation.createWorld(glm::vec3(20.0f, 20.0f, 20.0f));

    Lattice::LuaState luaState;
    expect(luaState.valid(), "Lua state should be created");
    luaState.bindSimulation(simulation);

    const bool ok = luaState.runString(R"(
        local count, names = scene:load_molecules("Mods/Base/Molecules")
        assert(count >= 2)
        assert(#names >= 2)
        assert(atoms.H == "H")
        assert(molecule.h2o == "h2o")
    )");
    expect(ok, luaState.lastError());
}

static void testLuaDslSimulationWorldGasBuildsScene() {
    Lattice::Simulation simulation;
    simulation.createWorld(glm::vec3(20.0f, 20.0f, 20.0f));

    Lattice::LuaState luaState;
    expect(luaState.valid(), "Lua state should be created");
    luaState.bindSimulation(simulation);

    const bool ok = luaState.runString(R"(
        dofile("Mods/Base/API/base.lua")

        simulation {
            world {
                name = "gas_mix",
                size = { 40, 40, 40 },
                content = {
                    load_molecules {
                        path = "Mods/Base/Molecules",
                    },
                    random_fill {
                        density = 0.01,
                        region = box {
                            size = fullworld - 4,
                            center = center,
                        },
                        composition = {
                            { name = molecule.h2o, fraction = 1.0 },
                        }
                    }
                }
            }
        }
    )");
    expect(ok, luaState.lastError());
    expect(simulation.worldTitle() == "gas_mix", "DSL world name should become world title");
    expect(simulation.atoms().size() > 0, "DSL gas world should spawn atoms");
}

static void testLuaDslSimulationWorldLatticeBuildsScene() {
    Lattice::Simulation simulation;
    simulation.createWorld(glm::vec3(60.0f, 60.0f, 60.0f));

    Lattice::LuaState luaState;
    expect(luaState.valid(), "Lua state should be created");
    luaState.bindSimulation(simulation);

    const bool ok = luaState.runString(R"(
        dofile("Mods/Base/API/base.lua")
        local spacing = scene:lj_min(atom.C, atom.C)

        simulation {
            world {
                name = "hex_lattice",
                size = { 60, 60, 60 },
                content = {
                    lattice_fill {
                        structure = "bcc",
                        region = box {
                            size = { spacing * 3 + 0.1, spacing * 3 + 0.1, spacing * 2 + 0.1 },
                            center = center,
                        },
                        margin = 0.0,
                        composition = {
                            { name = atom.C, fraction = 1.0 },
                        }
                    }
                }
            }
        }
    )");
    expect(ok, luaState.lastError());
    expect(simulation.worldTitle() == "hex_lattice", "DSL lattice world name should become world title");
    expect(simulation.atoms().size() > 0, "DSL BCC lattice world should spawn atoms");
}

static void testLuaDslSimulationWorldHexLatticeBuildsScene() {
    Lattice::Simulation simulation;
    simulation.createWorld(glm::vec3(60.0f, 60.0f, 60.0f));

    Lattice::LuaState luaState;
    expect(luaState.valid(), "Lua state should be created");
    luaState.bindSimulation(simulation);

    const bool ok = luaState.runString(R"(
        dofile("Mods/Base/API/base.lua")
        local spacing = scene:lj_min(atom.C, atom.C)

        simulation {
            world {
                name = "hex_packing",
                size = { 60, 60, 60 },
                content = {
                    lattice_fill {
                        structure = "hex",
                        region = box {
                            size = { spacing * 3 + 0.1, spacing * 3 + 0.1, spacing * 2 + 0.1 },
                            center = center,
                        },
                        margin = 0.0,
                        composition = {
                            { name = atom.C, fraction = 1.0 },
                        }
                    }
                }
            }
        }
    )");
    expect(ok, luaState.lastError());
    expect(simulation.worldTitle() == "hex_packing", "DSL hex lattice world name should become world title");
    expect(simulation.atoms().size() > 0, "DSL hex lattice world should spawn atoms");
}

int main() {
    registerClassicMDPlugin();
    testOctreeBuildChargeAndChildren();
    testCoulombFarFieldApproximation();
    testSpawnWaterMoleculeCreatesLocalAtoms();
    testSpawnNitrogenMoleculeCreatesStableBond();
    testSpawnAdditionalDiatomicMoleculesCreateStableBond();
    testCheckedMoleculeSpawnRejectsBlockedPoint();
    testWaterMoleculeBondsStayLocalAfterNeighborSort();
    testLuaSceneObjectLoadsMoleculesDirectory();
    testLuaDslSimulationWorldGasBuildsScene();
    testLuaDslSimulationWorldLatticeBuildsScene();
    testLuaDslSimulationWorldHexLatticeBuildsScene();
    std::cout << "All Lattice octree/Coulomb tests passed." << std::endl;
    return 0;
}
