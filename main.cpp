#include "App/Application.h"

int runApplication() {
    Application application;
    return application.run();
}

int main() { return runApplication(); }

#if defined(_WIN32)
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return runApplication(); }
#endif
