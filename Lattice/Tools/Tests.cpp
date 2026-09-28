#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Tools/Logger.hpp>
#include "Lattice/Tools/LogMode.hpp"
#include "Lattice/Tools/LogScope.hpp"


namespace Lattice {

namespace {
struct TestFailure : std::exception {
    const char* what() const noexcept override {
        return "test requirement failed";
    }
};

thread_local bool currentTestFailed = false;
}

void testRequire(bool condition, const char* expression, const char* file, int line) {
    if (condition)
        return;

    currentTestFailed = true;
    Logger::warning("Test", "REQUIRE failed: {} ({}:{})", expression, file, line);
    throw TestFailure{};
}

void testCheck(bool condition, const char* expression, const char* file, int line) {
    if (condition)
        return;

    currentTestFailed = true;
    Logger::warning("Test", "CHECK failed: {} ({}:{})", expression, file, line);
}

Test& Test::instance() {
    static Test blueprints;
    return blueprints;
}

int Test::runAll(LogMode mode) {
    int failed = 0;
    Logger::message("<mut>~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~</>");
    LogScope testing("Tests", mode, "<b>Running</>");
    for (TestCase& test : tests_) {
        auto& log = LogSystem::current();
        const LogMode effective = LogModes::inherit(mode, log.currentOrDefault());
        const size_t testDepth = log.scopeDepth() + 1;
        const size_t maxDepth = hasMode(effective, LogMode::Verbose)
            ? LogModes::UnlimitedDepth
            : testDepth + 1;

        LogScope testScope("Test", mode, maxDepth, "'{}'", test.name);
        currentTestFailed = false;
        try {
            auto fixture = test.createFixture();
            test.function(*fixture);
        } catch (const TestFailure&) {
        } catch (const std::exception& e) {
            currentTestFailed = true;
            Logger::exception("Test", "<err>threw: {}</>", e.what());
        } catch (...) {
            currentTestFailed = true;
            Logger::exception("Test", "<err>threw unknown exception</>");
        }
        if (currentTestFailed) {
            ++failed;
            if (!test.description.empty()) {
                Logger::warning("Desc", "<wrn>{}</>", test.description);
            }
            testScope.finishError("<err>'{}' failed</>", test.name);
        } else {
            testScope.finish("<ok><b>'{}' passed<//>", test.name);
        }
    }
    if (failed == 0) {
        testing.finish("all {} tests passed", tests_.size());
    } else {
        testing.finishError("<ok><b>{} passed,<//> <err>{} failure</>", tests_.size()-failed, failed);
    }
    return failed;
}

} // namespace Lattice
