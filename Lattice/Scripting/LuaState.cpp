#include "LuaState.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <string>

namespace Lattice {

LuaState::LuaState() {
    state_ = luaL_newstate();
    if (state_) {
        luaL_openlibs(state_);
    }
}

LuaState::~LuaState() { reset(); }

LuaState::LuaState(LuaState&& other) noexcept : state_(other.state_) { other.state_ = nullptr; }

LuaState& LuaState::operator=(LuaState&& other) noexcept {
    if (this != &other) {
        reset();
        state_ = other.state_;
        other.state_ = nullptr;
    }
    return *this;
}

bool LuaState::runString(std::string_view script) {
    if (!state_) {
        return false;
    }

    const std::string ownedScript(script);
    return luaL_dostring(state_, ownedScript.c_str()) == LUA_OK;
}

const char* LuaState::lastError() const noexcept {
    if (!state_) {
        return "Lua state is not initialized";
    }

    const char* message = lua_tostring(state_, -1);
    return message ? message : "";
}

void LuaState::reset() noexcept {
    if (state_) {
        lua_close(state_);
        state_ = nullptr;
    }
}

} // namespace Lattice
