#pragma once

#include <string_view>

class Simulation;
struct DebugViews;

void updateAtomSelectionDebug(const DebugViews& debugViews, const Simulation& simulation);
void updateSimulationDebug(const DebugViews& debugViews, const Simulation& simulation, std::string_view integratorName);
