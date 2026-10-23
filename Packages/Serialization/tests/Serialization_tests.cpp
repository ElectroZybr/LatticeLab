#include <chrono>
#include <fstream>
#include <iostream>
#include <Lattice/Composition.hpp>
#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/PackageManager.hpp>
#include <Serialization/Serializer.hpp>
#include <StdIo/IOSubsystem.hpp>
#include <StdIo/YamlParser.hpp>
#include <StdIo/LoaderAPI.hpp>
#include <type_traits>

using namespace Lattice;
namespace {
int constructions = 0;
struct Settings {
    Settings() { ++constructions; }
    double dt = 0.25;
    int iterations = 3;
    int untouched = 99;
    bool enabled = true;
    std::string label = "initial";
    glm::vec3 color{0};
    Array weights;
    Object metadata;
    void step() {}
};
void composeRun(NodeBuild root) {
    auto settings = root.emplace<Settings>("settings");
    settings.param("dt", &Settings::dt);
    settings.param("iterations", &Settings::iterations);
    settings.param("untouched", &Settings::untouched);
    settings.param("enabled", &Settings::enabled);
    settings.param("label", &Settings::label);
    settings.param("color", &Settings::color);
    settings.param("weights", &Settings::weights);
    settings.param("metadata", &Settings::metadata);
    settings.action("step", &Settings::step);
}
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class F> void rejects(F&& action) {
    bool failed = false;
    try { action(); } catch (const std::exception&) { failed = true; }
    check(failed, "Expected an error");
}
Object branch(Object params, bool blueprint = true) {
    return Object{{blueprint ? "TestRun:run" : "run", Object{{"settings", std::move(params)}}}};
}
Document configuration(Object params, bool blueprint = true) {
    return Document{Object{{"Lattice", branch(std::move(params), blueprint)},
        {"Unrelated", Object{{"keep", "preserved"}}}}};
}
Document tree(Object children) {
    return Document{Object{{"Lattice", std::move(children)}}};
}
Object& params(Document& document) {
    auto& root = *std::get<ObjectPtr>(*document.get("Lattice"));
    auto& run = *std::get<ObjectPtr>(root.at("TestRun:run"));
    return *std::get<ObjectPtr>(run.at("settings"));
}

struct Fixture {
    DLLoader libraries; // Unload only after destroying nodes and their blueprints.
    Context runtime;
    PackageManager packages{runtime.blueprints, libraries};
    NodeId root = InvalidNodeId;
    NodeId serializer = InvalidNodeId;
    NodeId ioBranch = InvalidNodeId;
    IOSubsystem* io = nullptr;
    std::filesystem::path directory;
    explicit Fixture(const std::filesystem::path& project) {
        packages.scanDirectory(project / "Packages/StdIo");
        packages.checkCandidates();
        check(packages.loadCandidates() == 1, "Failed to load the real StdIo package");
        runtime.blueprints.addComposition("TestRun", composeRun);
        root = runtime.nodes.factory.folder(InvalidNodeId, "root");
        ioBranch = runtime.nodes.builder.add(root, runtime.blueprints.resolve("StdIo"), "io-package");
        io = NodeBuild{root, runtime.nodes}.focus<IOSubsystem>().get();
        check(io != nullptr, "StdIo did not publish its service");
        serializer = findSerializer(ioBranch);
        directory = std::filesystem::temp_directory_path() /
            ("latticelab-serialization-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(directory);
    }
    ~Fixture() {
        runtime.nodes.ops.destroyBranch(root);
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
    }
    NodeId findSerializer(NodeId branch) {
        const auto id = runtime.nodes.registry.find("config", branch, runtime.blueprints.id<Serialization::Serializer>());
        check(id != InvalidNodeId, "StdIo did not compose its serializer");
        return id;
    }
    // Startup and users call load/save on the StdIo branch root.
    void action(std::string_view name, std::optional<std::string> file = {}) {
        auto& nodes = runtime.nodes;
        const auto id = nodes.exports.find(ioBranch, name);
        check(id != InvalidExportId, "Package action is missing");
        ActionContext context{ioBranch};
        if (file) {
            const Value value{*file};
            nodes.exports.invoke(id, context, std::span<const Value>{&value, 1});
        } else nodes.exports.invoke(id, context);
    }
    std::filesystem::path write(const Document& document, std::string_view name = "config.yaml") {
        const auto path = directory / name;
        io->write(path, document);
        return path;
    }
    void load(const Document& document) { action("load", write(document).string()); }
    Ref<Settings> settings() {
        const auto run = runtime.nodes.registry.find("run", root, runtime.blueprints.resolve("TestRun"));
        check(run != InvalidNodeId, "Run branch was not created");
        return NodeBuild{run, runtime.nodes}.require<Settings>("settings");
    }
};

void roundTrip(const std::filesystem::path& project) {
    Fixture f(project);
    const auto before = constructions;
    auto document = configuration(Object{{"dt", 0.125}, {"iterations", int64_t(7)},
        {"label", "quoted \"text\"\nnext line"}, {"color", Array{int64_t(1), 0.5, int64_t(0)}},
        {"weights", Array{int64_t(1), 2.5}}, {"metadata", Object{{"tags", Array{"a", "b"}}}}});
    f.load(document);
    auto settings = f.settings();
    check(constructions == before + 1 && settings->dt == 0.125 && settings->iterations == 7,
          "Missing branch was not built");
    check(settings->color == glm::vec3(1, .5, 0), "Vector parameter was not restored");
    check(settings->untouched == 99, "Unlisted parameter was changed");
    f.action("load");
    check(constructions == before + 1, "Reload duplicated an existing branch");
    f.runtime.nodes.exports.set(f.runtime.nodes.exports.find(settings.id(), "dt"), Value{0.5});
    settings->untouched = 123;
    f.runtime.nodes.exports.set(f.runtime.nodes.exports.find(settings.id(), "label"), Value{"saved"});
    f.action("save");
    auto saved = f.io->read(f.directory / "config.yaml");
    check(params(saved).at("dt").get<double>() == 0.5, "Current parameter was not saved");
    check(!params(saved).contains("untouched"), "Unlisted parameter leaked into config");
    check(saved.get("Unrelated")->require<Object>().at("keep").get<std::string>() == "preserved",
          "Unrelated section was lost");
    Fixture second(project);
    second.action("load", (f.directory / "config.yaml").string());
    check(second.settings()->dt == 0.5 && second.settings()->label == "saved", "Saved file did not restore");
}

void explicitLayer(const std::filesystem::path& project) {
    Fixture f(project);
    f.load(tree({}));
    auto& nodes = f.runtime.nodes;
    nodes.builder.add(f.root, nodes.blueprints.resolve("TestRun"), "run");
    auto settings = f.settings();
    const auto dt = nodes.exports.find(settings.id(), "dt");
    const auto untouched = nodes.exports.find(settings.id(), "untouched");
    const auto metadata = nodes.exports.find(settings.id(), "metadata");
    nodes.exports.set(dt, Value{.25});
    nodes.exports.set(untouched, Value{int64_t(42)});
    nodes.exports.set(metadata, Value{Object{{"nested", Array{"user"}}}});
    settings->untouched = 456;
    auto& nested = *std::get<ArrayPtr>(settings->metadata.at("nested"));
    *std::get<StringPtr>(nested.front()) = "internal";
    rejects([&] { nodes.exports.set(dt, Value{"wrong type"}); });
    f.action("save");
    auto saved = f.io->read(f.directory / "config.yaml");
    auto& values = params(saved);
    check(values.size() == 3 && values.at("dt").get<double>() == .25, "Explicit defaults or newly assigned params missing");
    check(values.at("untouched").get<int64_t>() == 42, "Direct field mutation leaked into the layer");
    check(values.at("metadata").require<Object>().at("nested").require<Array>().front().get<std::string>() == "user",
          "Nested layer values alias mutable object state");
    rejects([&] { f.action("save", (f.directory / "unsupported.tsv").string()); });
    Fixture second(project);
    second.action("load", (f.directory / "config.yaml").string());
    check(second.settings()->untouched == 42, "Newly discovered branch did not round-trip");
    nodes.exports.reset(untouched);
    nodes.exports.reset(metadata);
    f.action("save");
    saved = f.io->read(f.directory / "config.yaml");
    check(params(saved).size() == 1 && settings->untouched == 456, "Reset must remove only the override");
    nodes.exports.reset(dt);
    f.action("save");
    check(f.io->read(f.directory / "config.yaml").get("Lattice")->require<Object>().empty(), "Empty runtime layer was not pruned");
    nodes.ops.retireBranch(nodes.registry.require(settings.id()).parent);
    nodes.ops.maintain();
    nodes.builder.add(f.root, nodes.blueprints.resolve("TestRun"), "run");
    f.action("save");
    check(f.io->read(f.directory / "config.yaml").get("Lattice")->require<Object>().empty(), "Reused node inherited old overrides");
}

void unnamedBranches(const std::filesystem::path& project) {
    Fixture f(project);
    auto& nodes = f.runtime.nodes;
    nodes.blueprints.addComposition("OtherRun", composeRun);
    for (const auto* type : {"TestRun", "OtherRun"}) {
        const auto branch = nodes.builder.add(f.root, nodes.blueprints.resolve(type), "");
        const auto settings = NodeBuild{branch, nodes}.require<Settings>("settings");
        nodes.exports.set(nodes.exports.find(settings.id(), "iterations"), Value{int64_t(17)});
    }
    const auto file = f.directory / "unnamed.toml";
    f.action("save", file.string());
    auto saved = f.io->read(file);
    check(saved.get("Lattice")->require<Object>().size() == 2, "Unnamed branches were merged");
    Fixture second(project);
    second.runtime.blueprints.addComposition("OtherRun", composeRun);
    second.runtime.blueprints.addComposition("Namespace::TestRun", composeRun);
    second.action("load", file.string());
    for (const auto* type : {"TestRun", "OtherRun"}) {
        const auto branch = second.runtime.nodes.registry.find("", second.root, second.runtime.blueprints.find(type));
        const auto settings = NodeBuild{branch, second.runtime.nodes}.require<Settings>("settings");
        check(settings->iterations == 17, "Typed unnamed branch failed to restore");
    }
}

void compactNamesAndAmbiguity(const std::filesystem::path& project) {
    Fixture f(project);
    f.load(configuration({}));
    auto& nodes = f.runtime.nodes;
    const auto first = f.settings();
    const auto run = nodes.registry.require(first.id()).parent;
    struct OtherSettings { double dt = .1; };
    auto other = NodeBuild{run, nodes}.emplace<OtherSettings>("settings");
    other.param("dt", &OtherSettings::dt);
    rejects([&] { f.load(tree(Object{{"TestRun:run", Object{{"settings", Object{{"dt", .5}}}}}})); });
    const auto firstKey = nodes.blueprints.require(nodes.registry.require(first.id()).bp).name + ":settings";
    const auto otherKey = nodes.blueprints.require(nodes.registry.require(other.id()).bp).name + ":settings";
    f.load(tree(Object{{"TestRun:run", Object{{firstKey, Object{{"dt", .5}}}, {otherKey, Object{{"dt", .75}}}}}}));
    check(first->dt == .5 && other->dt == .75, "Typed keys did not resolve ambiguous siblings");
    f.action("save");
    auto saved = f.io->read(f.directory / "config.yaml");
    const auto& runConfig = saved.get("Lattice")->require<Object>().at("TestRun:run").require<Object>();
    check(runConfig.contains(firstKey) && runConfig.contains(otherKey) && !runConfig.contains("settings"),
          "Ambiguous siblings lost type qualification on save");

    // A parameter and a child may have the same name. Parameter identity wins
    // for plain keys; the child remains accessible through Type:name.
    auto build = NodeBuild{run, nodes};
    build.param("settings", first->untouched);
    f.load(tree(Object{{"TestRun:run", Object{{"settings", int64_t(123)}, {firstKey, Object{{"dt", .625}}}}}}));
    check(first->untouched == 123 && first->dt == .625, "Child/parameter collision was not resolved by typed key");
    f.action("save");
    saved = f.io->read(f.directory / "config.yaml");
    check(saved.get("Lattice")->require<Object>().at("TestRun:run").require<Object>().at("settings").get<int64_t>() == 123,
          "Parent parameter lost to a same-named child");
}

void existingAndTypes(const std::filesystem::path& project) {
    Fixture f(project);
    f.load(configuration(Object{{"dt", 1.0}}));
    f.load(configuration(Object{{"dt", int64_t(2)}, {"iterations", 8.0}}, false));
    check(f.settings()->dt == 2 && f.settings()->iterations == 8, "Numeric conversion failed");
    rejects([&] { f.load(configuration(Object{{"dt", "wrong"}})); });
    rejects([&] { f.load(configuration(Object{{"enabled", int64_t(1)}})); });
    rejects([&] { f.load(configuration(Object{{"iterations", 1.25}})); });
    rejects([&] { f.load(configuration(Object{{"color", Array{1.0, 2.0}}})); });
    rejects([&] { f.load(configuration(Object{{"missing", 1.0}})); });
    rejects([&] { f.load(configuration(Object{{"step", int64_t(1)}})); });
}

void treeErrors(const std::filesystem::path& project) {
    Fixture f(project);
    rejects([&] { f.load(configuration({}, false)); });
    auto doc = tree(Object{{"Missing:x", Object{}}});
    rejects([&] { f.load(doc); });
    f.load(configuration({}));
    doc = tree(Object{{"Serialization:run", Object{}}});
    rejects([&] { f.load(doc); });
    rejects([&] { f.load(Document{Object{{"Lattice", Object{{"typo", true}}}}}); });
    doc = tree(Object{{"run", Object{}}, {"TestRun:run", Object{}}});
    rejects([&] { f.load(doc); });
    const auto run = f.runtime.nodes.registry.find("run", f.root, f.runtime.blueprints.resolve("TestRun"));
    f.runtime.nodes.ops.retireBranch(run);
    rejects([&] { f.load(configuration({})); });
    f.runtime.nodes.ops.maintain();
    f.load(configuration(Object{{"dt", .75}}));
    check(f.settings()->dt == .75, "Missing branch was not restored after retirement");
}

void saveErrorsAndSaveAs(const std::filesystem::path& project) {
    Fixture f(project);
    f.action("save", (f.directory / "empty.yaml").string());
    check(f.io->read(f.directory / "empty.yaml").get("Lattice")->require<Object>().empty(), "Empty layer before load");
    f.load(configuration(Object{{"dt", .125}}));
    f.runtime.nodes.exports.set(f.runtime.nodes.exports.find(f.settings().id(), "dt"), Value{.5});
    const auto output = f.directory / "copy.yaml";
    f.action("save", output.string());
    auto original = f.io->read(f.directory / "config.yaml");
    check(params(original).at("dt").get<double>() == .125, "Save-as changed its source");
    const auto run = f.runtime.nodes.registry.find("run", f.root, f.runtime.blueprints.resolve("TestRun"));
    f.runtime.nodes.ops.retireBranch(run);
    f.runtime.nodes.ops.maintain();
    f.action("save");
    auto retained = f.io->read(output);
    check(retained.get("Lattice")->require<Object>().empty(), "Retired branch leaked into user layer");
}

void userDirectory(const std::filesystem::path& project) {
    Fixture f(project);
    std::filesystem::create_directory(f.directory / "Config");
    const auto staticFile = f.directory / "Config/runtime.yaml";
    std::ofstream(staticFile) << "Static: keep\n";
    std::filesystem::current_path(f.directory);
    try {
        f.action("load"); // User/ does not exist on a first launch.
        check(!std::filesystem::exists("User"), "Load unexpectedly wrote user data");
        f.action("save");
        check(std::filesystem::exists("User/runtime.yaml"), "Default save must create User/runtime.yaml");
        f.action("load");
        check(f.io->read("User/runtime.yaml").get("Lattice")->require<Object>().empty(), "First-run layer round-trip");
        check(f.io->read(staticFile).get("Static")->get<std::string>() == "keep", "Static Config was modified");
    } catch (...) {
        std::filesystem::current_path(project);
        throw;
    }
    std::filesystem::current_path(project);
}

void ioErrors(const std::filesystem::path& project) {
    Fixture f(project);
    const auto malformed = f.directory / "bad.yaml";
    { std::ofstream file(malformed); file << "[broken\n"; }
    rejects([&] { f.action("load", malformed.string()); });
    rejects([&] { f.action("load", (f.directory / "missing.yaml").string()); });
    rejects([&] { f.action("load", f.write(Document{Object{{"Other", true}}}).string()); });
    rejects([&] { f.io->read(f.directory / "unsupported.xyz"); });
    rejects([&] { f.io->write(f.directory / "readonly.tsv", Document{}); });
    const auto output = f.write(Document{Object{{"keep", true}}}, "preserved.yaml");
    rejects([&] { f.io->write(output, Document{Object{{"null", Value{}}}}); });
    check(f.io->read(output).get("keep")->get<bool>(), "Writer failure damaged an existing file");
}
void yamlIO(const std::filesystem::path& project) {
    Fixture f(project);
    // Test the default relative path without reading or changing user settings.
    std::filesystem::create_directory(f.directory / "User");
    f.io->write(f.directory / "User/runtime.yaml", tree({}));
    std::filesystem::current_path(f.directory);
    try { f.action("load"); }
    catch (...) { std::filesystem::current_path(project); throw; }
    std::filesystem::current_path(project);
    const auto input = f.directory / "manual.yml";
    {
        std::ofstream file(input);
        file << R"yaml(# Human-written YAML, not generated by the writer.
Lattice:
  TestRun:run:
    settings:
      dt: 0.2
      label: "true"
Extra:
  "null": "string key"
  quoted: ["true", "123", "1.0", "null", ""]
  native: [true, 123, 1.0, 1e2, 0x10, 0o10]
  forced: [!!str 123, !!float 1]
  min: -9223372036854775808
  max: 9223372036854775807
  message: |
    first line
    second line
  anchor: &pair {a: 1, b: 2}
  alias: *pair
)yaml";
    }
    f.action("load", input.string());
    check(f.settings()->dt == .2 && f.settings()->label == "true", "Human YAML configuration failed");
    auto document = f.io->read(input);
    const auto& extra = document.get("Extra")->require<Object>();
    for (const auto& item : extra.at("quoted").require<Array>())
        check(item.is<std::string>(), "Quoted YAML scalar lost its string type");
    const auto& native = extra.at("native").require<Array>();
    check(native[0].is<bool>() && native[1].is<int64_t>() && native[2].is<double>() &&
          native[3].get<double>() == 100 && native[4].get<int64_t>() == 16 && native[5].get<int64_t>() == 8,
          "YAML scalar types were not resolved");
    const auto& forced = extra.at("forced").require<Array>();
    check(forced[0].is<std::string>() && forced[1].is<double>(), "Explicit YAML tags were ignored");
    f.action("save");
    auto saved = f.io->read(input);
    {
        std::ifstream file(input);
        const std::string output{std::istreambuf_iterator<char>(file), {}};
        check(output.find("Lattice:") != std::string::npos && output.find("\"Lattice\":") == std::string::npos,
              "Ordinary YAML keys should not be quoted");
        check(output.find("\"null\":") != std::string::npos, "Reserved YAML keys must stay quoted");
    }
    const auto& savedExtra = saved.get("Extra")->require<Object>();
    check(savedExtra.at("null").get<std::string>() == "string key", "YAML writer changed a reserved-word key");
    check(savedExtra.at("native").require<Array>()[2].is<double>(), "Integral double lost its type on save");
    for (const auto& item : savedExtra.at("quoted").require<Array>())
        check(item.is<std::string>(), "YAML writer did not quote strings");
    check(savedExtra.at("message").get<std::string>() == "first line\nsecond line\n", "Block string changed");
    check(savedExtra.at("alias").require<Object>().at("b").get<int64_t>() == 2, "YAML alias did not round-trip");
    // Other existing formats remain available through the same IO API.
    const auto toml = f.write(configuration(Object{{"dt", .4}}), "other.toml");
    f.action("load", toml.string());
    check(f.settings()->dt == .4, "TOML IO regressed");

    for (const auto* invalid : {"x: null\n", "x: 1\nx: 2\n", "x: 9223372036854775808\n",
             "x: &cycle [*cycle]\n", "x: 1\n---\ny: 2\n", "[one, two]\n", "? [a, b]\n: 1\n"}) {
        std::ofstream file(input);
        file << invalid;
        file.close();
        rejects([&] { f.io->read(input); });
    }
}

void compositionLifetime(const std::filesystem::path& project) {
    Fixture f(project);
    f.load(configuration(Object{{"dt", .125}}));
    auto& nodes = f.runtime.nodes;
    const auto service = NodeBuild{f.root, nodes}.focus<IOSubsystem>().ref();
    const auto yaml = nodes.registry.find("yaml", f.ioBranch, f.runtime.blueprints.id<YamlParser>());
    // Parsers are configured values of IO, not dependencies: removing one only shrinks the set.
    check(!nodes.dependencies.retained(service.id(), yaml), "IO retained a configured parser");
    check(nodes.dependencies.retained(f.serializer, service.id()), "Serializer did not retain IO");
    nodes.ops.retireBranch(yaml);
    nodes.ops.maintain();
    check(nodes.registry.require(service.id()).state == NodeState::Active, "IO retired with one of its parsers");
    rejects([&] { f.action("load"); });

    // IO itself is required: its removal retires the serializer built on it.
    nodes.ops.retireBranch(f.ioBranch);
    check(nodes.registry.require(f.serializer).state == NodeState::Retiring, "Serializer survived removal of IO");
    check(nodes.exports.find(f.serializer, "load") == InvalidExportId, "Retiring serializer still exports load");
    nodes.ops.maintain();
    check(!nodes.registry.get(f.serializer), "Serializer was not retired");
    check(f.settings()->dt == .125, "Unrelated simulation branch was removed");
    f.ioBranch = nodes.builder.add(f.root, f.runtime.blueprints.resolve("StdIo"), "io-package");
    f.io = NodeBuild{f.root, nodes}.focus<IOSubsystem>().get();
    f.serializer = f.findSerializer(f.ioBranch);
    f.load(configuration(Object{{"dt", .75}}));
    check(f.settings()->dt == .75, "Rebuilt IO/Serialization did not work");
}

void separateIOBranches(const std::filesystem::path& project) {
    Fixture f(project);
    auto& nodes = f.runtime.nodes;
    const auto first = detail::reference<IOSubsystem>(nodes, f.ioBranch);
    const auto other = nodes.builder.add(f.root, f.runtime.blueprints.resolve("StdIo"), "other-io");
    const auto second = detail::reference<IOSubsystem>(nodes, other);
    check(nodes.dependencies.retained(f.findSerializer(other), second.id()), "Serializer did not use its branch IO");
    check(nodes.dependencies.retained(f.serializer, first.id()), "Existing serializer was rebound");
}

}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Pass the LatticeLab project directory\n"; return 2; }
    try {
        Logger::setDefaultMode(LogMode::Clean | LogMode::OnlyWarn);
        const auto project = std::filesystem::absolute(argv[1]);
        std::filesystem::current_path(project); // Match the application's Config/ working directory.
        roundTrip(project);
        explicitLayer(project);
        unnamedBranches(project);
        compactNamesAndAmbiguity(project);
        existingAndTypes(project);
        treeErrors(project);
        saveErrorsAndSaveAs(project);
        userDirectory(project);
        ioErrors(project);
        yamlIO(project);
        compositionLifetime(project);
        separateIOBranches(project);
        std::cout << "Serialization: 12 integration scenarios passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
