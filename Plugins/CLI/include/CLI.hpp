#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Tools/Logger.hpp>


class CLI final : public ServiceAPI {
    ExportsView exports;
    LogSystem::SinkId logSink = 0;

public:
    explicit CLI(NodeBuild branch) {}

    void configure(NodeConfigure branch) {
        exports = branch.exports();
    }

    void run() override {
        // LogSystem::setConsoleOutput(false);

        logSink = LogSystem::addSink([this](const LogEvent& event) {
            onLog(event);
        });

        while (!stopRequested()) {
            // input/event loop
        }

        LogSystem::removeSink(logSink);
        // LogSystem::setConsoleOutput(true);
    }

private:
    void onLog(const LogEvent& event) {
        Logger::info("CLI", "{}", event.text.render());
    }
};