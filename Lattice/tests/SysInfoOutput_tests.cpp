#include <Lattice/Tools/SysInfo/Output.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {

TEST(SysInfoOutput_FormatsOperatingSystem, Fixture) {
    const SystemInfo::MachineInfo machine{
        .os = {
            .name = "Test OS 1.0",
            .kernel = "6.12.0",
            .architecture = "x86_64"
        },
        .build = {
            .executable = "benchmark-runner",
            .versionMajor = 1,
            .versionMinor = 2,
            .versionPatch = 3,
            .compiler = SystemInfo::Compiler::GCC,
            .compilerMajor = 14,
            .compilerMinor = 2,
            .compilerPatch = 1,
            .build = SystemInfo::BuildType::Debug
        }
    };

    const TextFormatter output = SystemInfo::Output::format(machine);

    REQUIRE(output.diagnostics().empty());
    REQUIRE(output.plain().find("System") != std::string::npos);
    REQUIRE(output.plain().find("Test OS 1.0") != std::string::npos);
    REQUIRE(output.plain().find("kernel: 6.12.0") != std::string::npos);
    REQUIRE(output.plain().find("arch: x86_64") != std::string::npos);
    REQUIRE(output.plain().find("Build") != std::string::npos);
    REQUIRE(output.plain().find("benchmark-runner 1.2.3") != std::string::npos);
    REQUIRE(output.plain().find("GCC 14.2.1") != std::string::npos);
    REQUIRE(output.plain().find("type: debug") != std::string::npos);
}

TEST(SystemInfo_CollectsOperatingSystem, Fixture) {
    const SystemInfo::OsInfo os = SystemInfo::collectOS();

    REQUIRE(!os.name.empty());
    REQUIRE(!os.architecture.empty());
}

TEST(SystemInfo_CollectsBuild, Fixture) {
    const SystemInfo::BuildInfo build = SystemInfo::collectBuild();

    REQUIRE(!build.executable.empty());
    REQUIRE(build.compiler != SystemInfo::Compiler::Unknown);
    REQUIRE(build.build != SystemInfo::BuildType::Unknown);
}

TEST(SysInfoOutput_SortsCpuCaches, Fixture) {
    const SystemInfo::MachineInfo machine{
        .processors = {
            {
                .id = 0,
                .name = "CPU",
                .caches = {
                    {.level = 3, .type = SystemInfo::CacheType::Unified},
                    {.level = 1, .type = SystemInfo::CacheType::Instruction},
                    {.level = 2, .type = SystemInfo::CacheType::Unified},
                    {.level = 1, .type = SystemInfo::CacheType::Data}
                }
            }
        }
    };

    const std::string output = SystemInfo::Output::format(machine).plain();
    const size_t l1d = output.find("L1D");
    const size_t l1i = output.find("L1I");
    const size_t l2 = output.find("L2");
    const size_t l3 = output.find("L3");

    REQUIRE(l1d != std::string::npos);
    REQUIRE(l1i != std::string::npos);
    REQUIRE(l2 != std::string::npos);
    REQUIRE(l3 != std::string::npos);
    REQUIRE(l1d < l1i);
    REQUIRE(l1i < l2);
    REQUIRE(l2 < l3);
}

}
