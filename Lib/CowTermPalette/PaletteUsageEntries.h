#pragma once

#include <FrecencySearch/LearnedPicks.h>
#include <Miro/Reflect.h>

#include <cstdint>
#include <string>

namespace term
{
struct UsageEntry
{
    double count = 0;
    std::int64_t at = 0;

    MIRO_REFLECT(count, at)
};

struct LearnedPickEntry
{
    std::string query;
    std::string destinationKey;
    double count = 0;
    std::int64_t at = 0;

    MIRO_REFLECT(query, destinationKey, count, at)
};

frecencysearch::LearnedPick toLearnedPick(const LearnedPickEntry& entry);

// The entry's count decayed to `nowMs`.
double calculateStoredUsage(const UsageEntry& entry, std::int64_t nowMs);
} // namespace term
