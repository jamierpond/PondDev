#pragma once

#include <Miro/Reflect.h>

#include <cstdint>

namespace grazer
{
struct Stamp
{
    std::int64_t at = 0;

    // Uses so far, each 1, halving every frecencysearch::usageHalfLifeMs, as of `at`.
    double usage = 0;

    MIRO_REFLECT(at, usage)
};
} // namespace grazer
