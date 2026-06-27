#include "App/Application.h"

int runApplication(int, char**) {
    Application application;
    return application.run();
}

int main(int argc, char** argv) { return runApplication(argc, argv); }

#if defined(_WIN32)
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return runApplication(__argc, __argv); }
#endif
