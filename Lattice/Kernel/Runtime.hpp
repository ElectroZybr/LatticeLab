#pragma once

#include <filesystem>
#include <string>

#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/SubsystemAPI.hpp>
#include <Lattice/Kernel/PluginManager.hpp>
#include <Lattice/Kernel/StartupConfig.hpp>
#include <Lattice/Kernel/Requirements.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/Bindings.hpp>
#include "Lattice/Kernel/DLLoader.hpp"
#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/Model.hpp>
#include <Lattice/Tools/SystemInfo.hpp>
#include "Lattice/Kernel/Objects.hpp"
#include "Lattice/Tools/LogScope.hpp"
#include "Lattice/Tools/LogMode.hpp"
#include "Lattice/Tools/Logger.hpp"
#include "Lattice/Tools/Tests.hpp"


namespace Lattice {

class Runtime {
    static constexpr std::string_view tag = "Runtime";
public:
    Runtime() : root(run_ctx, nullptr)
              , pluginManager(run_ctx.blueprints, dlLoader) {
        // регистрация интерфейсов ядра
        run_ctx.blueprints.add<ServiceAPI>();
        run_ctx.blueprints.add<SubsystemAPI>();
        run_ctx.blueprints.add<Model, ServiceAPI>();
    }

    void buildBranch(const StartupEntry& entry) {
        LogScope scope(tag, "Build branch '{}' with name '{}'", entry.type, entry.name);

        Node& node = root.addNode(entry.type, entry.name);

        if (entry.host) {
            if (host)
                throw Lattice::Exception(tag, "Runtime already has a host service");

            host = &root.require(entry.type, entry.name);
            Logger::info(tag, "Host service '{}'", entry.type);
        }

        node.configureBranch();

        scope.finish("Build '{}' done", entry.type);
    }

    void startService(const StartupEntry& entry) {
        if (!entry.enabled || entry.host)
            return;

        Node& service = root.require(entry.type, entry.name);

        const ObjectId serviceApiId = root.findBlueprint<ServiceAPI>();
        if (!run_ctx.blueprints.isA(service.getBlueprintId(), serviceApiId))
            return;

        auto* api = service.get<ServiceAPI>();
        if (!api)
            throw Lattice::Exception(tag, "Service '{}' has no object", entry.type);

        api->start();

        Logger::info(tag, "Started service '{}.{}'", entry.type, entry.name);
    }

    void run(int argc, char** argv) {
        try {
            Logger::setDefaultMode(LogMode::Clean | LogMode::OnlyWarn);
            Lattice::CliSystemInfo::printSystemInfo();
            std::filesystem::path configPath = "lattice.toml";
            bool testMode = false, benchMode = false;

            for (int i = 1; i < argc; ++i) {
                const std::string_view arg = argv[i];
                if (arg == "--verbose" || arg == "-v") {
                    Logger::setDefaultMode(LogMode::Verbose | LogMode::Gap);
                } else if (arg == "--config" || arg == "-c") {
                    if (++i >= argc)
                        throw Lattice::Exception(tag, "missing path for {}", arg);
                    configPath = argv[i];
                } else if (arg == "--tests" || arg == "-t") {
                    testMode = true;
                }
            }

            StartupConfig config(configPath);

            { // инициализация ядра
                LogScope scope(tag, "<b>System loading</>");
                // загрузка плагинов
                pluginManager.load("Plugins");
                scope.finish("<b>Loaded</>");
            }
            
            if (testMode) { // режим прогона тестов
                dlLoader.load("Lattice", ".tests");
                dlLoader.load("Plugins", ".tests");
                TestBlueprints::instance().runAll();
                return;
            }

            if (benchMode) {}
            
            { // Сборка дерева компонентов
                LogScope scope(tag, "<b>System build</>");
                for (const auto& entry : config.entries()) {
                    if (entry.enabled)
                        buildBranch(entry);
                }
                scope.finish("<b>Build finished</>");
            }

            // { // связывание компонентов
            //     LogScope scope(tag, "<b>System configuring</>");
            //     root.configureAll();
            //     scope.finish("<b>Configuration finished</>");
            // }

            { // стартовые данные после configure всех веток
                LogScope scope(tag, "<b>System boot</>");
                loadStartup();
                scope.finish("<b>Boot finished</>");
            }

            { // запуск сервисов
                LogScope scope(tag, "<b>System start</>");
                for (const auto& entry : config.entries())
                    if (entry.enabled)
                        startService(entry);
                scope.finish("<b>Start finished</>");
            }
            
            run_ctx.printTree();
            root.dumpTree();
            run_ctx.blueprints.dumpTree();
            // root.requireContext().(0, 65);
            // Logger::message("{}", kernel.objects.stringPath(17));

            if (host) {
                host->get<ServiceAPI>()->enter();
            } else {
                while (running) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
            stopAll();
        } catch (const std::exception& error) {
            reportException(error);
        } catch (...) {
            reportUnknownException();
        }
    }

    void stop(std::string_view instanceName) {
        Slot<ServiceAPI> service = root.find<ServiceAPI>(instanceName);

        if (!service)
            return;

        service->stop();

        if (service.node == host)
            host = nullptr;

        root.remove<ServiceAPI>(instanceName);
    }

    ~Runtime() {
        stopAll();
    }

    void reportException(const std::exception& error) const {
        auto* fatal = dynamic_cast<const Lattice::Exception*>(&error);
        Logger::message("\n<r>~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~</>");
        if (fatal) {
            Logger::exception(fatal->tag(), "{}", error.what());
            Logger::message("Dump components tree (failed node is red):");
            root.dumpTree();
            // run_ctx.blueprints.dumpTree();
        } else {
            Logger::exception(tag, "Unhandled exception: {}", error.what());
            Logger::message("Dump components tree:");
            root.dumpTree();
        }
        Logger::message("<r><b>Critical error. Application terminated.<//>");
        Logger::message("Crash log: {}", std::string(LogSystem::getPath()));
        Logger::message("<r>~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~</>\n");
    }

    void reportUnknownException() const {
        Logger::exception(tag, "Unhandled non-standard exception");
        Logger::message("Dump components tree");
        root.dumpTree();
    }

private:
    void loadStartup() {
        const ObjectId id = run_ctx.find("load");
        if (!Objects::valid(id)) {
            Logger::info(tag, "no load action, skip startup config");
            return;
        }

        Logger::info(tag, "loading startup config");
        run_ctx.bindings.invoke(id);
    }

    void stopAll() {
        running = false;
        root.stopServices();
    }

    DLLoader dlLoader;
    Context run_ctx;
    Node root;
    PluginManager pluginManager;

    bool running = true;
    Node* host = nullptr;
};
}
