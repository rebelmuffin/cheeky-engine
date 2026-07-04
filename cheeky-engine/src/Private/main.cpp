#include "CVars.h"
#include "EngineCore.h"

#include <cstdio>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    CVars cvars{};
    cvars.ReadFromFile("../.cvars");

    EngineCore engine(cvars);
    if (engine.InitialisationFailed())
    {
        std::cout << "Engine Initialisation Failed. Closing application..." << std::endl;
        return -1;
    }

    engine.RunMainLoop();

    return 0;
}
