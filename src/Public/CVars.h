#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>

struct CVars
{
    int32_t width = 1700;
    int32_t height = 900;
    float backbuffer_scale = 1.0f;
    bool use_validation_layers = true;
    bool force_immediate_uploads = false;

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
            int32_t _width, _height;
            if (std::sscanf(line.data(), "OVERRIDE_WINDOW_SIZE=%dx%d", &_width, &_height) == 2)
            {
                width = _width;
                height = _height;
                total_read += 2;
                continue;
            }

            if (std::sscanf(line.data(), "BACKBUFFER_SCALE=%f", &backbuffer_scale) == 2)
            {
                total_read++;
                continue;
            }

            if (std::strstr(line.data(), "USE_VALIDATION_LAYERS=false") ||
                std::strstr(line.data(), "USE_VALIDATION_LAYERS=0"))
            {
                use_validation_layers = false;
                total_read++;
                continue;
            }

            if (std::strstr(line.data(), "FORCE_IMMEDIATE_UPLOADS=true") ||
                std::strstr(line.data(), "FORCE_IMMEDIATE_UPLOADS=1"))
            {
                force_immediate_uploads = true;
                total_read++;
                continue;
            }
        }

        return total_read;
    }
};