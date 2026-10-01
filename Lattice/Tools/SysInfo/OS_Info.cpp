#include "SystemInfo.hpp"

#include <cstddef>
#include <fstream>
#include <string_view>

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <sys/utsname.h>
#endif

namespace Lattice::SystemInfo {

namespace {

std::string trim(std::string_view value) {
    const size_t first = value.find_first_not_of(" \t\r\n");

    if (first == std::string_view::npos)
        return {};

    const size_t last = value.find_last_not_of(" \t\r\n");
    return std::string(value.substr(first, last - first + 1));
}

std::string unquote(std::string value) {
    if (
        value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))
    ) {
        value = value.substr(1, value.size() - 2);
    }

    return value;
}

std::string linuxName() {
#if defined(__linux__)
    std::ifstream file("/etc/os-release");
    std::string prettyName;
    std::string name;
    std::string version;
    std::string line;

    while (std::getline(file, line)) {
        const size_t equals = line.find('=');

        if (equals == std::string::npos)
            continue;

        const std::string key = trim(std::string_view(line).substr(0, equals));
        const std::string value = unquote(
            trim(std::string_view(line).substr(equals + 1))
        );

        if (key == "PRETTY_NAME")
            prettyName = value;
        else if (key == "NAME")
            name = value;
        else if (key == "VERSION")
            version = value;
        else if (key == "VERSION_ID" && version.empty())
            version = value;
    }

    if (!prettyName.empty())
        return prettyName;

    if (!name.empty() && !version.empty())
        return name + " " + version;

    if (!name.empty())
        return name;
#endif

    return {};
}

std::string fallbackName() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "Unknown OS";
#endif
}

std::string fallbackArchitecture() {
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#else
    return "unknown";
#endif
}

}

OsInfo collectOS() {
    OsInfo info;
    info.name = linuxName();

    if (info.name.empty())
        info.name = fallbackName();

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
    utsname system{};

    if (uname(&system) == 0) {
        info.kernel = system.release;
        info.architecture = system.machine;
    }
#endif

    if (info.architecture.empty())
        info.architecture = fallbackArchitecture();

    return info;
}

}
