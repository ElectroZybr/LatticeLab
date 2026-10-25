// Benchmarks object without plugin loading.
#include <iostream>
#include <stdexcept>
#include <string>
#include <latticebench/Benchmarks.hpp>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::string text(const Lattice::ActionContext& context) {
    std::string result;
    for (const auto& item : context.output())
        if (const auto* value = std::get_if<Lattice::Value>(&item))
            result += value->toString();
    return result;
}
}

int main() {
    Benchmarks bench;
    Lattice::ActionContext listed;
    bench.list(listed);
    check(text(listed).find("Benchmarks") != std::string::npos, "list output");

    Lattice::ActionContext missing;
    bool failed = false;
    try { bench.view(missing, "nothing-1"); } catch (const Lattice::ExceptionBase&) { failed = true; }
    check(failed, "unknown result is rejected");
    check(bench.resultCount() == 0, "no results yet");
    std::cout << "Bench: list and result lookup passed\n";
}
