#include "ChickenCam.h"
#include "Logger.h"
#include <memory>
#include <thread>
#include <chrono>

int main(int argc, char* argv[])
{
    Logger::SetLogLevel(
        LogLevel::Critical |
        LogLevel::Error |
        LogLevel::Warning |
        LogLevel::Info |
        LogLevel::Debug);
    std::unique_ptr<ChickenCam> chickenCam = std::make_unique<ChickenCam>();
    chickenCam->Init();
    chickenCam->Run();
    return 0;
}