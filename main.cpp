#include "App/Application.h"
#include "Lattice/Log.hpp"

#include <string_view>

int runApplication(int argc, char** argv) {
    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        }
    }

    Log::setVerbose(verbose);
    Application application;
    return application.run();
}

int main(int argc, char** argv) { return runApplication(argc, argv); }

#if defined(_WIN32)
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return runApplication(__argc, __argv); }
#endif
