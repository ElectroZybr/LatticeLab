#include "LuaState.h"

#include <filesystem>
#include <utility>

#include <sol/sol.hpp>

#include "Lattice/Scripting/ScriptAPI.hpp"

namespace Lattice {

struct LuaStateImpl {
    sol::state lua;
    std::unique_ptr<ScriptAPI> api;
};

LuaState::LuaState() : impl_(std::make_unique<LuaStateImpl>()) {
    impl_->lua.open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::coroutine,
        sol::lib::string,
        sol::lib::os,
        sol::lib::math,
        sol::lib::table,
        sol::lib::io,
        sol::lib::utf8
    );
    state_ = impl_->lua.lua_state();
}

LuaState::~LuaState() { reset(); }

LuaState::LuaState(LuaState&& other) noexcept
    : impl_(std::move(other.impl_)), state_(other.state_), lastError_(std::move(other.lastError_)) {
    other.state_ = nullptr;
}

LuaState& LuaState::operator=(LuaState&& other) noexcept {
    if (this != &other) {
        reset();
        impl_ = std::move(other.impl_);
        state_ = other.state_;
        lastError_ = std::move(other.lastError_);
        other.state_ = nullptr;
    }
    return *this;
}

bool LuaState::runString(std::string_view script) {
    if (!impl_) {
        lastError_ = "Lua state is not initialized";
        return false;
    }

    const sol::protected_function_result result = impl_->lua.safe_script(std::string(script), sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error error = result;
        lastError_ = error.what();
        return false;
    }

    lastError_.clear();
    return true;
}

bool LuaState::runFile(const std::filesystem::path& scriptPath) {
    if (!impl_) {
        lastError_ = "Lua state is not initialized";
        return false;
    }

    const sol::load_result loaded = impl_->lua.load_file(scriptPath.string());
    if (!loaded.valid()) {
        const sol::error error = loaded;
        lastError_ = error.what();
        return false;
    }

    const sol::protected_function script = loaded;
    const sol::protected_function_result result = script();
    if (!result.valid()) {
        const sol::error error = result;
        lastError_ = error.what();
        return false;
    }

    lastError_.clear();
    return true;
}

void LuaState::bindSimulation(Simulation& simulation) {
    if (!impl_) {
        return;
    }

    impl_->api = std::make_unique<ScriptAPI>(simulation);

    impl_->lua.new_usertype<ScriptBatch>("ScriptBatch",
        "random_spawn", &ScriptBatch::random_spawn,
        "finish", &ScriptBatch::finish
    );

    impl_->lua.new_usertype<ScriptAPI>("ScriptAPI",
        "clear", &ScriptAPI::clear,
        "set_box", &ScriptAPI::set_box,
        "load_molecules", &ScriptAPI::load_molecules,
        "begin_batch", &ScriptAPI::begin_batch
    );

    impl_->lua["scene"] = std::ref(*impl_->api);
}

const char* LuaState::lastError() const noexcept {
    return lastError_.c_str();
}

void LuaState::reset() noexcept {
    impl_.reset();
    state_ = nullptr;
    lastError_.clear();
}

} // namespace Lattice
