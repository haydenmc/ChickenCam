#include "ChickenCam.h"
#include <memory>
#include <thread>
#include <chrono>

int main(int argc, char* argv[])
{
    std::unique_ptr<ChickenCam> chickenCam = std::make_unique<ChickenCam>();
    chickenCam->Run();
    return 0;
}