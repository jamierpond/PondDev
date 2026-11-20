#include "RankingMaterials.h"

#include <FrecencySearch/MatchLevel.h>

namespace term
{
PaletteUsage makePaletteUsage(const std::map<std::string, UsageEntry>& usageByKey,
                              const std::vector<LearnedPickEntry>& learnedPicks,
                              const std::string& query,
                              std::int64_t nowMs)
{
    auto usages = std::map<std::string, double> {};

    for (auto& [key, entry]: usageByKey)
        usages[key] = calculateStoredUsage(entry, nowMs);

    auto picks = std::vector<frecencysearch::LearnedPick> {};

    for (auto& entry: learnedPicks)
        picks.push_back(toLearnedPick(entry));

    auto usage = [usages = std::move(usages)](const std::string& key)
    {
        auto found = usages.find(key);
        return found != usages.end() ? found->second : 0.0;
    };

    auto normalisedQuery = frecencysearch::normaliseQuery(query);
    return {usage, frecencysearch::findLearnedPicksForQuery(picks, normalisedQuery, nowMs)};
}

std::vector<PaletteItem> orderPaletteItemsFromMaterials(const PaletteRankingMaterials& materials,
                                                        const std::string& query,
                                                        std::int64_t nowMs)
{
    auto usage = makePaletteUsage(materials.usageByKey, materials.learnedPicks, query, nowMs);
    return orderPaletteItems(materials.items, query, usage);
}
} // namespace term
