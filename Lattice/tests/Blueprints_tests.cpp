#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {
namespace {
struct BlueprintsFixture : TestFixture { Blueprints blueprints; };

template<typename F>
bool rejects(F&& operation) {
    try { operation(); }
    catch (const ExceptionBase&) { return true; }
    return false;
}
}

TEST(Blueprints_Namespaces, BlueprintsFixture) {
    auto& types = fixture.blueprints;
    const auto gpu = types.add("GPU::Device");
    const auto wgpu = types.add("Backend::WGPU::Device", {gpu});
    const auto plain = types.add("Root");
    REQUIRE(gpu != wgpu);
    REQUIRE(types.find("Device", "GPU") == gpu);
    REQUIRE(types.find("Backend::WGPU::Device") == wgpu);
    REQUIRE(types.find("Device", "Backend::WGPU") == wgpu);
    REQUIRE(types.find("Device") == Blueprints::InvalidId);
    REQUIRE(types.require(wgpu).shortName() == "Device");
    REQUIRE(types.require(wgpu).namespaceName() == "Backend::WGPU");
    REQUIRE(types.require(plain).namespaceName().empty());
    REQUIRE(types.resolve("Root") == plain);
    REQUIRE(types.resolve("Backend::WGPU::Device") == wgpu);
    REQUIRE(types.resolve("Missing") == Blueprints::InvalidId);
    REQUIRE(rejects([&] { types.resolve("Device"); }));
    REQUIRE(rejects([&] { types.add("GPU::Device"); }));
    REQUIRE(types.size() == 3);
    // Глобальное имя тоже участвует в проверке неоднозначности.
    types.add("Device");
    REQUIRE(rejects([&] { types.resolve("Device"); }));
}

TEST(Blueprints_InheritanceGraph, BlueprintsFixture) {
    auto& types = fixture.blueprints;
    const auto root = types.add("API::Root");
    const auto gpu = types.add("GPU::Device", {root});
    const auto compute = types.add("Compute::Device", {root});
    const auto device = types.add("WGPU::Device", {gpu, compute, gpu});
    REQUIRE(types.require(device).bases.size() == 2);
    REQUIRE(types.isA(device, gpu));
    REQUIRE(types.isA(device, compute));
    REQUIRE(types.isA(device, root));
    REQUIRE(types.isA(device, device));
    REQUIRE(!types.isA(gpu, compute));
    REQUIRE(!types.isA(root, device));
    REQUIRE(rejects([&] { types.addBase(root, device); }));
    REQUIRE(rejects([&] { types.addBase(device, device); }));
    REQUIRE(types.require(root).bases.empty());
    types.addBase(device, gpu);
    REQUIRE(types.require(device).bases.size() == 2);
    const auto extra = types.add("Extra");
    types.addBase(root, extra);
    REQUIRE(types.isA(device, extra));
}

TEST(Blueprints_InvalidInput, BlueprintsFixture) {
    auto& types = fixture.blueprints;
    const auto type = types.add("Valid");
    for (const auto name : {"", "::Device", "GPU::", "GPU::::Device", "GPU:Device", "GPU::Bad Name"})
        REQUIRE(rejects([&] { types.add(name); }));
    REQUIRE(rejects([&] { types.add("Broken", {Blueprints::InvalidId}); }));
    REQUIRE(rejects([&] { types.addBase(type, Blueprints::InvalidId); }));
    REQUIRE(rejects([&] { types.addBase(123, type); }));
    REQUIRE(rejects([&] { types.require(123); }));
    REQUIRE(types.get(Blueprints::InvalidId) == nullptr);
    REQUIRE(!types.isA(Blueprints::InvalidId, Blueprints::InvalidId));
    REQUIRE(!types.isA(type, 123));
    REQUIRE(types.size() == 1);
    REQUIRE(types.find("Broken") == Blueprints::InvalidId);
}
}
