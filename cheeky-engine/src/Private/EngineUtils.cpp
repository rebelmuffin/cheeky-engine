#include "EngineUtils.h"

#include <SDL3/SDL.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/vec2.hpp>

#include <cmath>
#include <cstring>

namespace
{
    inline uint64_t rotl64(uint64_t x, int8_t r) { return (x << r) | (x >> (64 - r)); }

    inline uint64_t fmix64(uint64_t k)
    {
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53ULL;
        k ^= k >> 33;
        return k;
    }

    inline uint64_t getblock64(const uint8_t* p)
    {
        uint64_t v;
        std::memcpy(&v, p, sizeof(v));
        return v;
    }
} // namespace

namespace EngineUtils
{
    float NormaliseAngleRadians(float theta)
    {
        constexpr float two_pi = glm::pi<float>() * 2.0f;
        return theta - two_pi * std::floor((theta + glm::pi<float>()) / two_pi);
    }

    uint32_t Hash(uint32_t x)
    {
        x = ((x >> 16) ^ x) * 0x45d9f3bu;
        x = ((x >> 16) ^ x) * 0x45d9f3bu;
        return (x >> 16) ^ x;
    }

    uint64_t Hash(uint64_t x)
    {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return x;
    }

    uint64_t Hash(std::string_view str, uint32_t seed)
    {
        // MurmurHash3 x64 128-bit

        const uint8_t* data = reinterpret_cast<const uint8_t*>(str.data());
        const int nblocks = static_cast<int>(str.size() / 16);

        uint64_t h1 = seed;
        uint64_t h2 = seed;

        constexpr uint64_t c1 = 0x87c37b91114253d5ULL;
        constexpr uint64_t c2 = 0x4cf5ad432745937fULL;

        // body
        for (int i = 0; i < nblocks; ++i)
        {
            uint64_t k1 = getblock64(data + i * 16);
            uint64_t k2 = getblock64(data + i * 16 + 8);

            k1 *= c1;
            k1 = rotl64(k1, 31);
            k1 *= c2;
            h1 ^= k1;

            h1 = rotl64(h1, 27);
            h1 += h2;
            h1 = h1 * 5 + 0x52dce729;

            k2 *= c2;
            k2 = rotl64(k2, 33);
            k2 *= c1;
            h2 ^= k2;

            h2 = rotl64(h2, 31);
            h2 += h1;
            h2 = h2 * 5 + 0x38495ab5;
        }

        // tail
        const uint8_t* tail = data + nblocks * 16;

        uint64_t k1 = 0;
        uint64_t k2 = 0;

        // Build k1 (bytes 0-7)
        if (str.size() & 15)
        {
            const size_t tailSize = str.size() & 15;

            if (tailSize > 0)
                k1 |= uint64_t(tail[0]);
            if (tailSize > 1)
                k1 |= uint64_t(tail[1]) << 8;
            if (tailSize > 2)
                k1 |= uint64_t(tail[2]) << 16;
            if (tailSize > 3)
                k1 |= uint64_t(tail[3]) << 24;
            if (tailSize > 4)
                k1 |= uint64_t(tail[4]) << 32;
            if (tailSize > 5)
                k1 |= uint64_t(tail[5]) << 40;
            if (tailSize > 6)
                k1 |= uint64_t(tail[6]) << 48;
            if (tailSize > 7)
                k1 |= uint64_t(tail[7]) << 56;

            // Build k2 (bytes 8-15)
            if (tailSize > 8)
                k2 |= uint64_t(tail[8]);
            if (tailSize > 9)
                k2 |= uint64_t(tail[9]) << 8;
            if (tailSize > 10)
                k2 |= uint64_t(tail[10]) << 16;
            if (tailSize > 11)
                k2 |= uint64_t(tail[11]) << 24;
            if (tailSize > 12)
                k2 |= uint64_t(tail[12]) << 32;
            if (tailSize > 13)
                k2 |= uint64_t(tail[13]) << 40;
            if (tailSize > 14)
                k2 |= uint64_t(tail[14]) << 48;

            if (tailSize > 8)
            {
                k2 *= c2;
                k2 = rotl64(k2, 33);
                k2 *= c1;
                h2 ^= k2;
            }

            if (tailSize > 0)
            {
                k1 *= c1;
                k1 = rotl64(k1, 31);
                k1 *= c2;
                h1 ^= k1;
            }
        }

        // finalization
        h1 ^= str.size();
        h2 ^= str.size();

        h1 += h2;
        h2 += h1;

        h1 = fmix64(h1);
        h2 = fmix64(h2);

        h1 += h2;
        // h2 += h1;

        return h1;
    }
} // namespace EngineUtils