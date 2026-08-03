#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>

namespace Cheeky
{
    struct Colour
    {
        uint8_t r, g, b, a;

        constexpr Colour(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) :
            r(r),
            g(g),
            b(b),
            a(a)
        {
        }

        explicit constexpr Colour(const uint32_t colour) :
            r((colour & 0xFF000000) >> 24),
            g((colour & 0x00FF0000) >> 16),
            b((colour & 0x0000FF00) >> 8),
            a(colour & 0x000000FF)
        {
        }

        static constexpr Colour FromRGBA(const float r, const float g, const float b, const float a)
        {
            return Colour{ static_cast<uint8_t>(r * 255.0f),
                           static_cast<uint8_t>(g * 255.0f),
                           static_cast<uint8_t>(b * 255.0f),
                           static_cast<uint8_t>(a * 255.0f) };
        }
        static constexpr Colour FromVec4(const glm::vec4& vec)
        {
            return FromRGBA(vec.x, vec.y, vec.z, vec.w);
        }

        [[nodiscard]] constexpr glm::vec4 ToVec4() const
        {
            return glm::vec4{ float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f };
        }
        [[nodiscard]] constexpr glm::vec3 ToVec3() const
        {
            return glm::vec3{ float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f };
        }
        [[nodiscard]] constexpr uint32_t To32Bit() const
        {
            const uint32_t rgba = r << 24 | g << 16 | b << 8 | a;
            return rgba;
        }
    };

    inline Colour Lerp(const float alpha, const Colour a, const Colour b)
    {
        const uint8_t out_r = a.r + static_cast<uint8_t>(static_cast<float>(b.r - a.r) * alpha);
        const uint8_t out_g = a.g + static_cast<uint8_t>(static_cast<float>(b.g - a.g) * alpha);
        const uint8_t out_b = a.b + static_cast<uint8_t>(static_cast<float>(b.b - a.b) * alpha);
        const uint8_t out_a = a.a + static_cast<uint8_t>(static_cast<float>(b.a - a.a) * alpha);
        return Colour{ out_r, out_g, out_b, out_a };
    }
} // namespace Cheeky
