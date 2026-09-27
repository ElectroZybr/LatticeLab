#pragma once

#include <atomic>
#include <filesystem>
#include <csignal>
#include <functional>
#include <string>
#include <vector>

#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Kernel/PluginManager.hpp>
#include <Lattice/Kernel/StartupConfig.hpp>
#include <Lattice/Kernel/Requirements.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/DLLoader.hpp>
#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/BlueprintRegister.hpp>
#include <Lattice/Kernel/Model.hpp>
#include <Lattice/Tools/SystemInfo.hpp>
#include "Lattice/Tools/LogScope.hpp"
#include "Lattice/Tools/LogMode.hpp"
#include "Lattice/Tools/Logger.hpp"
#include "Lattice/Tools/Tests.hpp"


namespace Lattice {

class Runtime {
    class Root {
    public:
        struct Desc {
            std::function<void()> dumpBlueprints;
            std::function<void()> requestExit;
        };

        Root(::NodeBuild node, const Desc& desc) {
            node.globalAction("dumpBlueprints", desc.dumpBlueprints);
            const ExportId quit = node.globalAction("quit", desc.requestExit);
            node.globalAlias("exit", quit);
        }
    };

    struct StartupBranch {
        NodeId node = InvalidNodeId;
        std::string type;
        std::string name;
        bool host = false;
    };

    static constexpr std::string_view tag = "Runtime";
    Context run_ctx;
    NodeId root = InvalidNodeId;
    NodeId host = InvalidNodeId;
    DLLoader dlLoader;
    PluginManager pluginManager;
    std::atomic<bool> running{true};
    inline static volatile std::sig_atomic_t interrupted = 0;
    
public:
    Runtime() : pluginManager(run_ctx.blueprints, dlLoader) {
        // регистрация интерфейсов ядра
        BlueprintRegister::add<Component>(run_ctx.blueprints);
        BlueprintRegister::add<ServiceAPI>(run_ctx.blueprints);
        BlueprintRegister::add<SubsystemAPI>(run_ctx.blueprints);
        BlueprintRegister::add<Model, ServiceAPI>(run_ctx.blueprints);
        const BlueprintId rootBlueprint = BlueprintRegister::add<Root>(run_ctx.blueprints, "Root");

        const Root::Desc rootDesc{
            .dumpBlueprints = [this] { run_ctx.blueprints.dumpTree(); },
            .requestExit = [this] { requestExit(); }
        };
        root = run_ctx.nodes.builder.build(InvalidNodeId, rootBlueprint, DefaultInstanceName, &rootDesc);
    }

    int run(int argc, char** argv) {
        interrupted = 0;
        const auto handler = +[](int) { interrupted = 1; };
        const auto previousInt = std::signal(SIGINT, handler);
        const auto previousTerm = std::signal(SIGTERM, handler);
        struct RestoreSignals {
            decltype(previousInt) interruptHandler;
            decltype(previousTerm) terminateHandler;
            ~RestoreSignals() {
                std::signal(SIGINT, interruptHandler);
                std::signal(SIGTERM, terminateHandler);
            }
        } restoreSignals{previousInt, previousTerm};
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
                return Test::instance().runAll() == 0 ? 0 : 1;
            }

            if (benchMode) {}
            
            const std::vector<StartupBranch> startupBranches = build(config);

            { // стартовые данные после configure всех веток
                LogScope scope(tag, "<b>System boot</>");
                loadStartup();
                scope.finish("<b>Boot finished</>");
            }

            { // запуск сервисов
                LogScope scope(tag, "<b>System start</>");
                for (const auto& branch : startupBranches)
                    startService(branch);
                scope.finish("<b>Start finished</>");
            }

            if (host != InvalidNodeId) {
                auto* service = run_ctx.nodes.configure(root).resolve<ServiceAPI>(host);
                if (!service) throw Exception(tag, "Host does not implement ServiceAPI");
                service->enter();
            } else {
                while (running && !interrupted)
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            stopAll();
            return 0;
        } catch (const std::exception& error) {
            reportException(error);
        } catch (...) {
            reportUnknownException();
        }
        stopAll();
        return 1;
    }

    void stop(std::string_view instanceName) {
        const NodeId id = run_ctx.nodes.configure(root).findId(typeKey<ServiceAPI>(), instanceName);
        auto service = run_ctx.nodes.configure(root).find<ServiceAPI>(instanceName);

        if (!service)
            return;

        service->stop();

        if (id == host)
            host = InvalidNodeId;

        run_ctx.nodes.ops.destroyBranch(id);
    }

    ~Runtime() {
        stopAll();
        run_ctx.nodes.ops.destroyBranch(root);
    }

    void reportException(const std::exception& error) const {
        auto* fatal = dynamic_cast<const Lattice::Exception*>(&error);
        Logger::message("\n<r>~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~</>");
        if (fatal) {
            Logger::exception(fatal->tag(), "{}", error.what());
            Logger::message("Dump components tree (failed node is red):");
            run_ctx.nodes.ops.dumpTree(root);
        } else {
            Logger::exception(tag, "Unhandled exception: {}", error.what());
            Logger::message("Dump components tree:");
            run_ctx.nodes.ops.dumpTree(root);
        }
        Logger::message("<r><b>Critical error. Application terminated.<//>");
        Logger::message("Crash log: {}", std::string(LogSystem::getPath()));
        Logger::message("<r>~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~</>\n");
    }

    void reportUnknownException() const {
        Logger::exception(tag, "Unhandled non-standard exception");
        Logger::message("Dump components tree");
        run_ctx.nodes.ops.dumpTree(root);
    }

private:
    std::vector<StartupBranch> build(const StartupConfig& config) {
        LogScope scope(tag, "<b>System build</>");

        std::vector<StartupBranch> branches;
        auto batch = run_ctx.nodes.builder.begin();
        NodeId startupHost = InvalidNodeId;

        for (const auto& entry : config.entries()) {
            if (!entry.enabled)
                continue;

            const BlueprintId blueprint = run_ctx.blueprints.resolve(entry.type);
            if (blueprint == InvalidBlueprintId)
                throw Exception(tag, "Unknown startup blueprint '{}'", entry.type);

            const NodeId node = batch.add(root, blueprint, entry.name);
            branches.push_back({node, entry.type, entry.name, entry.host});

            if (!entry.host)
                continue;

            if (startupHost != InvalidNodeId)
                throw Exception(tag, "Runtime already has a host service");

            startupHost = node;
            Logger::info(tag, "Host service '{}'", entry.type);
        }

        batch.commit();
        host = startupHost;
        scope.finish("<b>Build finished</>");
        return branches;
    }

    void startService(const StartupBranch& branch) {
        if (branch.host)
            return;

        auto* service = run_ctx.nodes.configure(root).resolve<ServiceAPI>(branch.node);
        if (!service)
            return;

        service->start();
        Logger::info(tag, "Started service '{}.{}'", branch.type, branch.name);
    }

    void requestExit() {
        running = false;

        if (host == InvalidNodeId)
            return;

        if (auto* service = run_ctx.nodes.configure(root).resolve<ServiceAPI>(host))
            service->stop();
    }

    void loadStartup() {
        // const ObjectId id = run_ctx.resolveFocus(InvalidFocusScopeId, run_ctx.roles.find("load"));
        // if (id == InvalidObjectId) {
        //     Logger::info(tag, "no load action, skip startup config");
        //     return;
        // }

        // Logger::info(tag, "loading startup config");
        // run_ctx.bindings.invoke(id);
    }

    void stopAll() {
        running = false;

        auto services = run_ctx.nodes.query.collect(
            root,
            run_ctx.blueprints.id<ServiceAPI>()
        );

        for (auto it = services.rbegin(); it != services.rend(); ++it)
            if (auto* service = run_ctx.nodes.configure(root).resolve<ServiceAPI>(*it))
                service->stop();
    }
};
}
