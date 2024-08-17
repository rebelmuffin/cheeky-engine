#include "EngineCore.h"
#include <iostream>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    int32_t width = 1700;
    int32_t height = 900;
    EngineCore engine(width, height);
    if (engine.InitialisationFailed())
    {
        std::cout << "Engine Initialisation Failed. Closing application..." << std::endl;
        return -1;
    }

    engine.RunMainLoop();

    return 0;
}
