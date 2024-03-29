#include "timer/rand.h"
#include <random>

uint32_t x = 998244353;
uint32_t y = 1366482745;
uint32_t z = 1508893039;
uint32_t w = 1812191717;

uint64_t splitmix64(uint64_t& seed)
{
    uint64_t s = seed += 0x9e3779b97f4a7c15;
    s = (s ^ (s >> 30)) * 0xbf58476d1ce4e5b9;
    s = (s ^ (s >> 27)) * 0x94d049bb133111eb;
    return s ^ (s >> 31);
}

void srand_(uint64_t seed) 
{
    uint64_t s = splitmix64(seed);
    x = uint32_t(s);
    y = uint32_t(s >> 32);

    uint64_t s2 = splitmix64(seed);
    z = uint32_t(s2);
    w = uint32_t(s2 >> 32);
}


uint32_t rand_() {
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}