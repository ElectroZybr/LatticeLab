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
#include <Lattice/Kernel/Settings.hpp>
#include "Lattice/Kernel/DLLoader.hpp"
#include <Lattice/Kernel/Kernel.hpp>
#include <Lattice/Kernel/Model.hpp>
#include <Lattice/Tools/SystemInfo.hpp>
#include "Lattice/Tools/LogScope.hpp"
#include "Lattice/Tools/LogMode.hpp"
#include "Lattice/Tools/Logger.hpp"
#include "Lattice/Tools/Tests.hpp"


namespace Lattice {

class Runtime {
    static constexpr std::string_view tag = "Runtime";
public:
    Runtime() : root(kernel, nullptr)
              , pluginManager(kernel.blueprints, dlLoader) {}

    void buildBranch(const StartupEntry& entry, std::string_view name = "default") {
        LogScope scope(tag, "Build branch '{}' with name '{}'", entry.name, name);
        if (kernel.blueprints.hasImpl<ServiceAPI>(entry.name)) {
            root.add<ServiceAPI>(entry.name, name);

            if (entry.host) {
                if (!hostName.empty())
                    throw Lattice::Exception(tag, "Runtime already has a host service");

                hostName = entry.name;
                Logger::info(tag, "Host service '{}'", entry.name);
            }

            scope.finish("Build '{}' done", entry.name);
            return;
        }

        if (kernel.blueprints.hasImpl<SubsystemAPI>(entry.name)) {
            root.add<SubsystemAPI>(entry.name, name);
            scope.finish("Build '{}' done", entry.name);
            return;
        }

        throw Lattice::Exception(tag, "unknown component '{}'", entry.name);
    }

    void startServices(const StartupConfig& config) {
        for (const auto& entry : config.entries()) {
            if (!entry.enabled || entry.host)
                continue;

            if (!kernel.blueprints.hasImpl<ServiceAPI>(entry.name))
                continue;

            auto service = root.require<ServiceAPI>(entry.name);
            service->start();

            Logger::info(tag, "Started service '{}'", entry.name);
        }
    }

    void run(int argc, char** argv) {
        try {
            Logger::setDefaultMode(LogMode::Clean);
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
                LogScope scope(tag, "<b>System launching</>");
                // регистрация интерфейсов ядра
                kernel.blueprints.registerAPI<ServiceAPI>();
                kernel.blueprints.registerAPI<SubsystemAPI>();
                // kernel.blueprints.registerImpl<SubsystemAPI, Model>();
                // загрузка плагинов
                pluginManager.loadPlugins("Plugins");
                scope.finish("<b>Launch finished</>");
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
                root.dumpTree();
                scope.finish("<b>Build finished</>");
            }

            { // связывание компонентов
                LogScope scope(tag, "<b>System configuring</>");
                root.configureAll();
                startServices(config);
                scope.finish("<b>Cofiguration finished</>");
            }
            
            root.dumpTree();
            kernel.blueprints.dumpTree();
            Logger::message("{}", kernel.objects.stringPath(17));

            if (!hostName.empty()) {
                auto host = root.require<ServiceAPI>(hostName);
                host->enter();
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
        root.remove<ServiceAPI>(instanceName);

        if (instanceName == hostName)
            hostName.clear();
    }

    ~Runtime() {
        stopAll();
    }

    Blueprints& blueprints() noexcept { return kernel.blueprints; }

    void reportException(const std::exception& error) const {
        auto* fatal = dynamic_cast<const Lattice::Exception*>(&error);
        Logger::message("\n<r>~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~</>");
        if (fatal) {
            Logger::exception(fatal->tag(), "{}", error.what());
            Logger::message("Dump components tree (failed node is red):");
            root.dumpTree(fatal->tag());
            kernel.blueprints.dumpTree();
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
        Logger::message("Dump components tree (failed node is red):");
        root.dumpTree();
    }

private:
    void stopAll() {
        running = false;
        root.stopServices();
    }

    Kernel kernel;

    DLLoader dlLoader;
    PluginManager pluginManager;
    Node root;

    bool running = true;
    std::string hostName;
};
}
