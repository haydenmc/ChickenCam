#include "ChickenCam.h"
#include <memory>

int main(int argc, char* argv[])
{
    std::unique_ptr<ChickenCam> chickenCam = std::make_unique<ChickenCam>();
    return 0;
}