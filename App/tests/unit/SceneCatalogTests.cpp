#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "App/GUI/interface/panels/io/ioPanelSceneCatalog.h"
#include "Lattice/tests/TestSupport.h"

namespace {
    struct ScopedTempDirectory {
        std::filesystem::path path;

        ScopedTempDirectory() {
            const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
            std::ostringstream name;
            name << "latticelab_scene_tests_" << now;

            path = std::filesystem::temp_directory_path() / name.str();
            std::filesystem::create_directories(path);
        }

        ~ScopedTempDirectory() { std::filesystem::remove_all(path); }
    };

    void writeTextFile(const std::filesystem::path& path, std::string_view content) {
        std::ofstream file(path, std::ios::trunc);
        expect(file.is_open(), "Test file should be created");
        file << content;
    }

    void testCreatedSceneAppearsInCatalog() {
        ScopedTempDirectory tempDir;
        const std::filesystem::path scenePath = tempDir.path / "methane.lat";

        writeTextFile(
            scenePath,
            "[meta]\n"
            "title Methane\n"
            "description Simple scene\n");

        const IOPanelSceneDirectory sceneDirectory{
            .path = tempDir.path,
            .source = IOPanelSceneSource::User,
        };
        const std::vector<IOPanelSceneTile> tiles = loadIOPanelSceneTiles({&sceneDirectory, 1});

        expect(tiles.size() == 1, "Created scene should appear in the catalog");
        expect(tiles.front().path == scenePath.string(), "Catalog should keep the created scene path");
        expect(tiles.front().title == "Methane", "Catalog should parse the scene title");
        expect(tiles.front().description == "Simple scene", "Catalog should parse the scene description");
        expect(tiles.front().writable, "User scene should be marked writable");
    }

    void testDeletedSceneDisappearsFromCatalog() {
        ScopedTempDirectory tempDir;
        const std::filesystem::path scenePath = tempDir.path / "temporary.lat";

        writeTextFile(
            scenePath,
            "[meta]\n"
            "title Temporary Scene\n");

        const IOPanelSceneDirectory sceneDirectory{
            .path = tempDir.path,
            .source = IOPanelSceneSource::User,
        };

        const std::vector<IOPanelSceneTile> tilesBeforeDelete = loadIOPanelSceneTiles({&sceneDirectory, 1});
        expect(tilesBeforeDelete.size() == 1, "Scene should exist before deletion");

        std::error_code removeError;
        const bool removed = std::filesystem::remove(scenePath, removeError);
        expect(!removeError && removed, "Scene file should be removed successfully");

        const std::vector<IOPanelSceneTile> tilesAfterDelete = loadIOPanelSceneTiles({&sceneDirectory, 1});
        expect(tilesAfterDelete.empty(), "Deleted scene should disappear from the catalog");
    }
}

void runAppTests() {
    testCreatedSceneAppearsInCatalog();
    testDeletedSceneDisappearsFromCatalog();
}
