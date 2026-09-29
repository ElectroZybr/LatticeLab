#pragma once

#include <atomic>
#include <filesystem>
#include <csignal>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Kernel/PluginManager.hpp>
#include <Lattice/Kernel/StartupConfig.hpp>
#include <Lattice/Kernel/Requirements.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Kernel/DLLoader.hpp>
#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/BlueprintRegister.hpp>
#include <Lattice/Kernel/BasicTable.hpp>
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
            std::function<void(ActionContext&, std::string, std::optional<std::string>)> addChild;
            std::function<void(ActionContext&, std::string, std::optional<std::string>)> delChild;
        };

        Root(::NodeBuild node, const Desc& desc) {
            node.globalAction("dumpBlueprints", desc.dumpBlueprints);
            const ExportId quit = node.globalAction("quit", desc.requestExit);
            node.globalAlias("exit", quit);
            node.globalAction<std::string, std::optional<std::string>>("add", desc.addChild);
            node.globalAction<std::string, std::optional<std::string>>("del", desc.delChild);
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
        BlueprintRegister::add<Table, Component>(run_ctx.blueprints);
        BlueprintRegister::add<BasicTable, Table>(run_ctx.blueprints);
        BlueprintRegister::add<ServiceAPI>(run_ctx.blueprints);
        BlueprintRegister::add<SubsystemAPI>(run_ctx.blueprints);
        BlueprintRegister::add<Model, ServiceAPI>(run_ctx.blueprints);
        const BlueprintId rootBlueprint = BlueprintRegister::add<Root>(run_ctx.blueprints, "Root");

        const Root::Desc rootDesc{
            .dumpBlueprints = [this] { run_ctx.blueprints.dumpTree(); },
            .requestExit = [this] { requestExit(); },
            .addChild = [this](ActionContext& context, std::string blueprint, std::optional<std::string> name) {
                addChild(context.node(), blueprint, name.value_or(std::string{}));
            },
            .delChild = [this](ActionContext& context, std::string blueprint, std::optional<std::string> name) {
                delChild(context.node(), blueprint, name);
            }
        };
        root = run_ctx.nodes.builder.add(InvalidNodeId, rootBlueprint, DefaultInstanceName, &rootDesc);
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
            CliSystemInfo::printSystemInfo();
            std::filesystem::path configPath = "lattice.toml";
            bool testMode = false, benchMode = false;

            for (int i = 1; i < argc; ++i) {
                const std::string_view arg = argv[i];
                if (arg == "--verbose" || arg == "-v") {
                    Logger::setDefaultMode(LogMode::Verbose | LogMode::Gap);
                } else if (arg == "--config" || arg == "-c") {
                    if (++i >= argc)
                        throw Exception<Runtime>("missing path for {}", arg);
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
                scope.finish("<ok>Loaded</>");
            }
            
            if (testMode) { // режим прогона тестов
                dlLoader.load("Lattice", ".tests");
                dlLoader.load("Plugins", ".tests");
                return Test::instance().runAll() == 0 ? 0 : 1;
            }

            if (benchMode) {}
            dlLoader.load("Lattice", ".bench");
            
            const std::vector<StartupBranch> startupBranches = build(config);

            { // стартовые данные после configure всех веток
                LogScope scope(tag, "<b>System boot</>");
                loadStartup();
                scope.finish("<ok>Boot finished</>");
            }

            { // запуск сервисов
                LogScope scope(tag, "<b>System start</>");
                for (const auto& branch : startupBranches)
                    startService(branch);
                scope.finish("<ok>Start finished</>");
            }

            if (host != InvalidNodeId) {
                auto* service = resolveService(host);
                if (!service) throw Exception<Runtime>("Host does not implement ServiceAPI");
                service->enter(run_ctx.nodes.ops);
                run_ctx.nodes.ops.maintain();
            } else {
                while (running && !interrupted) {
                    run_ctx.nodes.ops.maintain();
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
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
        const BlueprintId api = run_ctx.blueprints.id<ServiceAPI>();
        const NodeId id = run_ctx.nodes.query.find(root, api, instanceName);
        auto* service = id == InvalidNodeId
            ? nullptr
            : static_cast<ServiceAPI*>(run_ctx.nodes.query.resolve(id, api));

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
        auto* fatal = dynamic_cast<const ExceptionBase*>(&error);
        Logger::message("\n<err>~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~</>");
        if (fatal) {
            Logger::exception(fatal->tag(), "{}", error.what());
            Logger::message("Dump components tree (failed node is red):");
            run_ctx.nodes.ops.dumpTree(root);
        } else {
            Logger::exception(tag, "Unhandled exception: {}", error.what());
            Logger::message("Dump components tree:");
            run_ctx.nodes.ops.dumpTree(root);
        }
        Logger::message("<err>Critical error. Application terminated.</>");
        Logger::message("Crash log: {}", std::string(LogSystem::getPath()));
        Logger::message("<err>~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~</>\n");
    }

    void reportUnknownException() const {
        Logger::exception(tag, "Unhandled non-standard exception");
        Logger::message("Dump components tree");
        run_ctx.nodes.ops.dumpTree(root);
    }

private:
    ServiceAPI* resolveService(NodeId id) {
        return static_cast<ServiceAPI*>(
            run_ctx.nodes.query.resolve(id, run_ctx.blueprints.id<ServiceAPI>())
        );
    }

    NodeId addChild(NodeId parent, std::string_view blueprint, std::string_view name) {
        const BlueprintId id = run_ctx.blueprints.resolve(blueprint);
        if (id == InvalidBlueprintId)
            throw Exception<Runtime>("Unknown blueprint '{}'", blueprint);

        return run_ctx.nodes.builder.add(parent, id, name);
    }

    void delChild(
        NodeId parent,
        std::string_view blueprint,
        const std::optional<std::string>& name
    ) {
        const BlueprintId type = run_ctx.blueprints.resolve(blueprint);
        if (type == InvalidBlueprintId)
            throw Exception<Runtime>("Unknown blueprint '{}'", blueprint);

        NodeId selected = InvalidNodeId;
        for (NodeId child : run_ctx.nodes.registry.children(parent)) {
            const auto& node = run_ctx.nodes.registry.require(child);
            if (node.state == NodeState::Retiring ||
                !run_ctx.blueprints.isA(node.bp, type) ||
                (name && node.name != *name))
                continue;

            if (selected != InvalidNodeId)
                throw Exception<Runtime>(
                    "Child '{}' is ambiguous under node #{}; specify its name",
                    blueprint,
                    parent
                );

            selected = child;
        }

        if (selected == InvalidNodeId) {
            if (name)
                throw Exception<Runtime>(
                    "Child '{}:{}' not found under node #{}",
                    blueprint,
                    *name,
                    parent
                );

            throw Exception<Runtime>(
                "Child '{}' not found under node #{}",
                blueprint,
                parent
            );
        }

        run_ctx.nodes.builder.del(parent, selected);
    }

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
                throw Exception<Runtime>("Unknown startup blueprint '{}'", entry.type);

            const NodeId node = batch.add(root, blueprint, entry.name);
            branches.push_back({node, entry.type, entry.name, entry.host});

            if (!entry.host)
                continue;

            if (startupHost != InvalidNodeId)
                throw Exception<Runtime>("Runtime already has a host service");

            startupHost = node;
            Logger::info(tag, "Host service '{}'", entry.type);
        }

        batch.commit();
        host = startupHost;
        scope.finish("<ok>Build finished</>");
        return branches;
    }

    void startService(const StartupBranch& branch) {
        if (branch.host)
            return;

        auto* service = resolveService(branch.node);
        if (!service)
            return;

        service->start();
        Logger::info(tag, "Started service '{}.{}'", branch.type, branch.name);
    }

    void requestExit() {
        running = false;

        if (host == InvalidNodeId)
            return;

        if (auto* service = resolveService(host))
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
            if (auto* service = resolveService(*it))
                service->stop();
    }
};
}
