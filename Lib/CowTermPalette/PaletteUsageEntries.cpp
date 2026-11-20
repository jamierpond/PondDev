#include "PaletteUsageEntries.h"

namespace term
{
frecencysearch::LearnedPick toLearnedPick(const LearnedPickEntry& entry)
{
    return {entry.query, entry.destinationKey, entry.count, entry.at};
}

double calculateStoredUsage(const UsageEntry& entry, std::int64_t nowMs)
{
    return frecencysearch::calculateDecayedUsage(entry.count, entry.at, nowMs);
}
} // namespace term
