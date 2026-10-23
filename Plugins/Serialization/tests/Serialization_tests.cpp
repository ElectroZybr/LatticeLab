#include <chrono>
#include <fstream>
#include <iostream>
#include <Lattice/Composition.hpp>
#include <Lattice/Kernel/BlueprintRegister.hpp>
#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/PluginManager.hpp>
#include <IOSubsystem.hpp>

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
    IOSubsystem* io = nullptr;
    std::filesystem::path directory;
    explicit Fixture(const std::filesystem::path& project) {
        BlueprintRegister::add<Component>(runtime.blueprints);
        BlueprintRegister::add<SubsystemAPI, Component>(runtime.blueprints);
        plugins.scanDirectory(project / "Plugins/StdIo");
        plugins.scanDirectory(project / "Plugins/Serialization");
        plugins.checkCandidates();
        check(plugins.loadCandidates() == 2, "Failed to load real IO/Serialization plugins");
        runtime.blueprints.addComposition("TestRun", composeRun);
        root = runtime.nodes.factory.folder(InvalidNodeId, "root");
        const auto ioId = runtime.nodes.builder.add(root, runtime.blueprints.id<IOSubsystem>());
        io = static_cast<IOSubsystem*>(runtime.nodes.query.resolve(ioId, runtime.blueprints.id<IOSubsystem>()));
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
    std::filesystem::path write(const Document& document, std::string_view name = "config.toml") {
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
    auto saved = f.io->read(f.directory / "config.toml");
    check(params(saved).at("dt").get<double>() == 0.5, "Current parameter was not saved");
    check(!params(saved).contains("untouched"), "Unlisted parameter leaked into config");
    check(saved.get("Unrelated")->require<Object>().at("keep").get<std::string>() == "preserved",
          "Unrelated section was lost");
    Fixture second(project);
    second.action("load", (f.directory / "config.toml").string());
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
    const auto output = f.directory / "copy.toml";
    f.action("save", output.string());
    auto original = f.io->read(f.directory / "config.toml");
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
    const auto malformed = f.directory / "bad.toml";
    { std::ofstream file(malformed); file << "[broken\n"; }
    rejects([&] { f.action("load", malformed.string()); });
    rejects([&] { f.action("load", (f.directory / "missing.toml").string()); });
    rejects([&] { f.action("load", f.write(Document{Object{{"Other", true}}}).string()); });
    rejects([&] { f.io->read(f.directory / "unsupported.xyz"); });
    rejects([&] { f.io->write(f.directory / "readonly.tsv", Document{}); });
    const auto output = f.write(Document{Object{{"keep", true}}}, "preserved.toml");
    rejects([&] { f.io->write(output, Document{Object{{"null", Value{}}}}); });
    check(f.io->read(output).get("keep")->get<bool>(), "Writer failure damaged an existing file");
}
}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Pass the LatticeLab project directory\n"; return 2; }
    try {
        Logger::setDefaultMode(LogMode::Clean | LogMode::OnlyWarn);
        const std::filesystem::path project{argv[1]};
        roundTrip(project);
        existingAndTypes(project);
        treeErrors(project);
        saveErrorsAndSaveAs(project);
        ioErrors(project);
        std::cout << "Serialization: 5 integration scenarios passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
