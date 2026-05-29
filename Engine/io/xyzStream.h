#pragma once

#include <string>

class Simulation;

class xyzStream {
public:
    void Start(std::string path);
    void WriteFrame(const Simulation& simulation);
    void Stop();

private:
    std::string path_ = "";
    bool isStreaming = false;
};