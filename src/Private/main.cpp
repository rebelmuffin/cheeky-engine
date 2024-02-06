#include "EngineCore.h"

int main(int argc, char* argv[])
{
    uint32_t width = 1700;
    uint32_t height = 900;
    EngineCore engine(width, height);

    engine.RunMainLoop();

    return 0;
}
