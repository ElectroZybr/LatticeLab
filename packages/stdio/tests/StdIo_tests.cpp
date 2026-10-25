// No NodeSystem, builders, package loader or runtime registration in this executable.
#include <stdio/IOSubsystem.hpp>
#include <stdio/LoaderAPI.hpp>
#include <stdio/YamlParser.hpp>
#include <stdio/TomlParser.hpp>
#include <stdio/TsvParser.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class F> void rejects(F&& fn) {
    bool failed = false;
    try { fn(); } catch (const Lattice::ExceptionBase&) { failed = true; }
    check(failed, "Expected an ordinary IO error");
}
struct Loader final : LoaderAPI {
    int count = 0;
    std::string_view section() const override { return "Example"; }
    void load(const Lattice::Value& data) override { count = data.require<Lattice::Object>().at("count").get<int>(); }
};
}

int main() {
    const auto directory = std::filesystem::temp_directory_path() /
        ("stdio-plain-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    struct Cleanup {
        std::filesystem::path directory;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(directory, ignored); }
    } cleanup{directory};
    try {
        YamlParser yaml;
        TomlParser toml;
        TsvParser tsv;
        IOSubsystem io{{&yaml, &toml, &tsv}};
        Document input{Lattice::Object{{"Example", Lattice::Object{{"count", int64_t(7)}, {"text", "true"}}}}};
        for (const auto* name : {"sample.yaml", "sample.yml", "sample.toml"}) {
            const auto path = directory / name;
            io.write(path, input);
            auto document = io.read(path);
            Loader loader;
            loader.load(*document.section(loader.section()));
            check(loader.count == 7, "Standalone parser/loader round-trip failed");
            check(document.section("Example")->require<Lattice::Object>().at("text").is<std::string>(), "String type changed");
        }
        const auto table = directory / "data.tsv";
        { std::ofstream file(table); file << "[Dataset]\nname\tvalue\nrow\t2.5\n"; }
        check(io.read(table).get("Dataset") != nullptr, "Standalone TSV parser failed");
        rejects([&] { io.write(table, input); });
        rejects([&] { IOSubsystem invalid{{nullptr}}; });
        YamlParser second;
        IOSubsystem ambiguous{{&yaml, &second}};
        rejects([&] { ambiguous.read(directory / "sample.yaml"); });
        IOSubsystem empty{{}};
        rejects([&] { empty.read(directory / "sample.yaml"); });
        // A polymorphic parser can be owned by ordinary application code.
        std::unique_ptr<ParserAPI> owned = std::make_unique<YamlParser>();
        check(owned->supportsExtension(".yml"), "Standalone parser interface failed");
        std::cout << "StdIo: standalone parsers, explicit loaders and ambiguity checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
