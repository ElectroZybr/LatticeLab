#include "LuaState.h"

#include <filesystem>
#include <utility>

#include <sol/sol.hpp>

#include "Lattice/Engine/physics/Atom/AtomData.h"
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
        "spawn", &ScriptBatch::spawn,
        "random_spawn", &ScriptBatch::random_spawn,
        "finish", &ScriptBatch::finish
    );

    impl_->lua.new_usertype<ScriptAPI>("ScriptAPI",
        "clear", &ScriptAPI::clear,
        "set_world_title", &ScriptAPI::set_world_title,
        "set_box", &ScriptAPI::set_box,
        "world_size", &ScriptAPI::world_size,
        "load_molecules", &ScriptAPI::load_molecules,
        "begin_batch", &ScriptAPI::begin_batch,
        "lj_min", &ScriptAPI::lj_min,
        "random_fill", &ScriptAPI::random_fill,
        "lattice_fill", &ScriptAPI::lattice_fill
    );

    sol::table atoms = impl_->lua.create_named_table("atoms");
    for (size_t i = 0; i < static_cast<size_t>(AtomData::Type::COUNT); ++i) {
        const AtomData::Type type = static_cast<AtomData::Type>(i);
        const std::string_view symbol = AtomData::symbol(type);
        if (!symbol.empty()) {
            atoms[std::string(symbol)] = std::string(symbol);
        }
    }

    sol::object moleculeObject = sol::make_object(impl_->lua, impl_->lua.create_table());
    const std::filesystem::path moleculeModulePath = std::filesystem::path("Mods") / "Base" / "API" / "molecule.lua";
    if (std::filesystem::exists(moleculeModulePath)) {
        const sol::load_result loaded = impl_->lua.load_file(moleculeModulePath.string());
        if (loaded.valid()) {
            const sol::protected_function module = loaded;
            const sol::protected_function_result result = module();
            if (result.valid() && result.get_type() == sol::type::table) {
                moleculeObject = sol::make_object(impl_->lua, result.get<sol::table>());
            }
        }
    }

    impl_->lua["molecule"] = moleculeObject;
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
