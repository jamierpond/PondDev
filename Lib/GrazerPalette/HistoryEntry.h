#pragma once

#include <Miro/Reflect.h>

#include <cstdint>
#include <string>

namespace grazer
{
struct HistoryEntry
{
    std::string url;
    std::string title;
    std::int64_t lastVisit = 0;
    std::int64_t visits = 0;

    MIRO_REFLECT(url, title, lastVisit, visits)
};
} // namespace grazer
