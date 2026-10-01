#include "SystemInfo.hpp"

#include <charconv>
#include <filesystem>
#include <string_view>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "0.0.0"
#endif

namespace Lattice::SystemInfo {

namespace {

namespace fs = std::filesystem;

std::string executableName() {
#if defined(__linux__)
    std::error_code error;
    const fs::path executable = fs::read_symlink("/proc/self/exe", error);

    if (!error)
        return executable.filename().string();
#elif defined(_WIN32)
    std::vector<wchar_t> path(32768);
    const DWORD size = GetModuleFileNameW(
        nullptr,
        path.data(),
        static_cast<DWORD>(path.size())
    );

    if (size != 0 && size < path.size())
        return fs::path(path.data(), path.data() + size).filename().string();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> path(size);

    if (_NSGetExecutablePath(path.data(), &size) == 0)
        return fs::path(path.data()).filename().string();
#endif

    return "unknown";
}

void parseVersion(std::string_view version, BuildInfo& info) {
    uint32_t* parts[] = {
        &info.versionMajor,
        &info.versionMinor,
        &info.versionPatch
    };

    for (uint32_t* part : parts) {
        const size_t dot = version.find('.');
        const std::string_view value = version.substr(0, dot);

        std::from_chars(value.data(), value.data() + value.size(), *part);

        if (dot == std::string_view::npos)
            break;

        version.remove_prefix(dot + 1);
    }
}

}

BuildInfo collectBuild() {
    BuildInfo info;
    info.executable = executableName();
    parseVersion(BUILD_VERSION, info);

#if defined(__clang__)
    info.compiler = Compiler::Clang;
    info.compilerMajor = __clang_major__;
    info.compilerMinor = __clang_minor__;
    info.compilerPatch = __clang_patchlevel__;
#elif defined(__GNUC__)
    info.compiler = Compiler::GCC;
    info.compilerMajor = __GNUC__;
    info.compilerMinor = __GNUC_MINOR__;
    info.compilerPatch = __GNUC_PATCHLEVEL__;
#elif defined(_MSC_VER)
    info.compiler = Compiler::MSVC;
    info.compilerMajor = _MSC_VER / 100;
    info.compilerMinor = _MSC_VER % 100;
#endif

#if defined(NDEBUG)
    info.build = BuildType::Release;
#else
    info.build = BuildType::Debug;
#endif

    return info;
}

}
