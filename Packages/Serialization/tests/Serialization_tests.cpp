#include <chrono>
#include <fstream>
#include <iostream>
#include <Lattice/Composition.hpp>
#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/PluginManager.hpp>
#include <StdIo/IOSubsystem.hpp>
#include <StdIo/YamlParser.hpp>
#include <StdIo/LoaderAPI.hpp>
#include <type_traits>

using namespace Lattice;
static_assert(!std::is_base_of_v<Component, ParserAPI>);
static_assert(!std::is_base_of_v<Component, LoaderAPI>);
static_assert(!std::is_base_of_v<Component, IOSubsystem>);
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
    Object settings{{"name", "settings"}, {"params", std::move(params)}};
    Object run{{"name", "run"}, {"children", Array{settings}}};
    if (blueprint) run.emplace("blueprint", "TestRun");
    return Object{{"children", Array{run}}};
}
Document configuration(Object params, bool blueprint = true) {
    return Document{Object{{"Lattice", branch(std::move(params), blueprint)},
        {"Unrelated", Object{{"keep", "preserved"}}}}};
}
Document tree(Array children) {
    Object root;
    root.emplace("children", std::move(children));
    return Document{Object{{"Lattice", std::move(root)}}};
}
Object& params(Document& document) {
    auto& root = *std::get<ObjectPtr>(*document.get("Lattice"));
    auto& run = *std::get<ObjectPtr>(std::get<ArrayPtr>(root.at("children"))->front());
    auto& settings = *std::get<ObjectPtr>(std::get<ArrayPtr>(run.at("children"))->front());
    return *std::get<ObjectPtr>(settings.at("params"));
}

struct Fixture {
    DLLoader libraries; // Unload only after destroying nodes and their blueprints.
    Context runtime;
    PluginManager plugins{runtime.blueprints, libraries};
    NodeId root = InvalidNodeId;
    NodeId serializer = InvalidNodeId;
    NodeId ioBranch = InvalidNodeId;
    IOSubsystem* io = nullptr;
    std::filesystem::path directory;
    explicit Fixture(const std::filesystem::path& project) {
        plugins.scanDirectory(project / "Packages/StdIo");
        plugins.scanDirectory(project / "Packages/Serialization");
        plugins.checkCandidates();
        check(plugins.loadCandidates() == 2, "Failed to load real IO/Serialization plugins");
        runtime.blueprints.addComposition("TestRun", composeRun);
        root = runtime.nodes.factory.folder(InvalidNodeId, "root");
        ioBranch = runtime.nodes.builder.add(root, runtime.blueprints.resolve("StdIo"), "io-package");
        io = runtime.nodes.configure(root).focus<IOSubsystem>().get();
        check(io != nullptr, "StdIo did not publish its service");
        serializer = runtime.nodes.builder.add(root, runtime.blueprints.resolve("Serialization"), "serialization");
        directory = std::filesystem::temp_directory_path() /
            ("latticelab-serialization-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(directory);
    }
    ~Fixture() {
        runtime.nodes.ops.destroyBranch(root);
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
    }
    void action(std::string_view name, std::optional<std::string> file = {}) {
        auto& nodes = runtime.nodes;
        const auto id = nodes.exports.find(serializer, name);
        check(id != InvalidExportId, "Plugin action is missing");
        ActionContext context{serializer};
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
          "Missing branch was not built/configured");
    check(settings->color == glm::vec3(1, .5, 0), "Vector parameter was not restored");
    check(settings->untouched == 99, "Unlisted parameter was changed");
    f.action("load");
    check(constructions == before + 1, "Reload duplicated an existing branch");
    settings->dt = 0.5;
    settings->untouched = 123;
    settings->label = "saved";
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
    auto doc = tree(Array{Object{{"name", "x"}, {"blueprint", "Missing"}}});
    rejects([&] { f.load(doc); });
    f.load(configuration({}));
    doc = tree(Array{Object{{"name", "run"}, {"blueprint", "Serialization"}}});
    rejects([&] { f.load(doc); });
    rejects([&] { f.load(Document{Object{{"Lattice", Object{{"typo", true}}}}}); });
    doc = tree(Array{Object{{"name", "run"}}, Object{{"name", "run"}}});
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
    rejects([&] { f.action("save"); });
    f.load(configuration(Object{{"dt", .125}}));
    f.settings()->dt = .5;
    const auto output = f.directory / "copy.yaml";
    f.action("save", output.string());
    auto original = f.io->read(f.directory / "config.yaml");
    check(params(original).at("dt").get<double>() == .125, "Save-as changed its source");
    const auto run = f.runtime.nodes.registry.find("run", f.root, f.runtime.blueprints.resolve("TestRun"));
    f.runtime.nodes.ops.retireBranch(run);
    f.runtime.nodes.ops.maintain();
    rejects([&] { f.action("save"); });
    auto retained = f.io->read(output);
    check(params(retained).at("dt").get<double>() == .5, "Failed save corrupted its file");
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
    f.action("load"); // The default Config/runtime.yaml uses the registered YAML parser.
    const auto input = f.directory / "manual.yml";
    {
        std::ofstream file(input);
        file << R"yaml(# Human-written YAML, not generated by the writer.
Lattice:
  children:
    - name: run
      blueprint: TestRun
      children:
        - name: settings
          params:
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
    const auto service = nodes.configure(f.root).focus<IOSubsystem>().ref();
    const auto yaml = nodes.registry.find("yaml", f.ioBranch, f.runtime.blueprints.id<YamlParser>());
    check(nodes.dependencies.retained(service.id(), yaml), "IO did not retain its parser dependency");
    check(nodes.dependencies.retained(f.serializer, service.id()), "Serialization branch did not retain shared IO");
    nodes.ops.retireBranch(yaml);
    check(nodes.registry.require(service.id()).state == NodeState::Retiring, "IO survived removal of a required parser");
    check(nodes.registry.require(f.serializer).state == NodeState::Retiring, "Serialization survived removal of IO");
    check(nodes.exports.find(f.serializer, "load") == InvalidExportId, "Retiring composition still exports load");
    nodes.ops.maintain();
    check(!nodes.registry.get(f.serializer), "Serialization branch was not retired");
    check(f.settings()->dt == .125, "Unrelated simulation branch was removed");
    nodes.ops.retireBranch(f.ioBranch);
    nodes.ops.maintain();
    f.ioBranch = nodes.builder.add(f.root, f.runtime.blueprints.resolve("StdIo"), "io-package");
    f.io = nodes.configure(f.root).focus<IOSubsystem>().get();
    f.serializer = nodes.builder.add(f.root, f.runtime.blueprints.resolve("Serialization"), "serialization");
    f.load(configuration(Object{{"dt", .75}}));
    check(f.settings()->dt == .75, "Rebuilt IO/Serialization did not work");
}

void sharedIOChoice(const std::filesystem::path& project) {
    Fixture f(project);
    auto& nodes = f.runtime.nodes;
    const auto first = nodes.configure(f.root).focus<IOSubsystem>().ref();
    const auto other = nodes.builder.add(f.root, f.runtime.blueprints.resolve("StdIo"), "other-io");
    const auto second = NodeBuild{other, nodes}.require<IOSubsystem>("io");
    rejects([&] { nodes.builder.add(f.root, f.runtime.blueprints.resolve("Serialization"), "other-config"); });
    NodeBuild{f.root, nodes}.focus<IOSubsystem>(second);
    const auto selected = nodes.builder.add(f.root, f.runtime.blueprints.resolve("Serialization"), "other-config");
    check(nodes.dependencies.retained(selected, second.id()), "Context selection did not reach Serialization");
    check(nodes.dependencies.retained(f.serializer, first.id()), "Existing dependency snapshot was rebound");
}

}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Pass the LatticeLab project directory\n"; return 2; }
    try {
        Logger::setDefaultMode(LogMode::Clean | LogMode::OnlyWarn);
        const auto project = std::filesystem::absolute(argv[1]);
        std::filesystem::current_path(project); // Match the application's Config/ working directory.
        roundTrip(project);
        existingAndTypes(project);
        treeErrors(project);
        saveErrorsAndSaveAs(project);
        ioErrors(project);
        yamlIO(project);
        compositionLifetime(project);
        sharedIOChoice(project);
        std::cout << "Serialization: 8 integration scenarios passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
