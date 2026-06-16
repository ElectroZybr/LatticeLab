#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

struct lua_State;

namespace Lattice {

class Simulation;
struct LuaStateImpl;

class LuaState {
public:
    LuaState();
    ~LuaState();

    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    LuaState(LuaState&& other) noexcept;
    LuaState& operator=(LuaState&& other) noexcept;

    [[nodiscard]] lua_State* raw() noexcept { return state_; }
    [[nodiscard]] const lua_State* raw() const noexcept { return state_; }

    [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }
    [[nodiscard]] bool runString(std::string_view script);
    [[nodiscard]] bool runFile(const std::filesystem::path& scriptPath);
    void bindSimulation(Simulation& simulation);
    [[nodiscard]] const char* lastError() const noexcept;

private:
    void reset() noexcept;

    std::unique_ptr<LuaStateImpl> impl_;
    lua_State* state_ = nullptr;
    std::string lastError_;
};

} // namespace Lattice
