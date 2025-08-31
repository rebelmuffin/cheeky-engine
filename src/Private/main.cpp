#include "EngineCore.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

struct CVars
{
    int32_t override_width;
    int32_t override_height;

    uint32_t ReadFromFile(std::filesystem::path path)
    {
        std::ifstream stream(path);
        std::string line;

        if (stream.is_open() == false)
        {
            return 0;
        }

        uint32_t total_read = 0;
        while (std::getline(stream, line))
        {
            int32_t width, height;
            if (std::sscanf(line.data(), "OVERRIDE_WINDOW_SIZE=%dx%d", &width, &height) == 2)
            {
                override_width = width;
                override_height = height;
                total_read += 2;
            }
        }

        return total_read;
    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    int32_t width = 1700;
    int32_t height = 900;

    CVars cvars{};
    cvars.ReadFromFile("../.cvars");

    if (cvars.override_height != 0 && cvars.override_width != 0)
    {
        width = cvars.override_width;
        height = cvars.override_height;
    }

    EngineCore engine(width, height);
    if (engine.InitialisationFailed())
    {
        std::cout << "Engine Initialisation Failed. Closing application..." << std::endl;
        return -1;
    }

    engine.RunMainLoop();

    return 0;
}
