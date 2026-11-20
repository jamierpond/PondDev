#include "RankingMaterials.h"

#include <FrecencySearch/LearnedPicks.h>

namespace grazer
{
double calculateStampUsage(const Stamp& stamp, std::int64_t nowMs)
{
    return frecencysearch::calculateDecayedUsage(stamp.usage, stamp.at, nowMs);
}

UsageFn makeUsageLookup(const std::map<std::string, Stamp>& stampsByNormalizedUrl,
                        std::int64_t nowMs)
{
    auto usages = std::map<std::string, double> {};

    for (auto& [normalizedUrl, stamp]: stampsByNormalizedUrl)
        usages[normalizedUrl] = calculateStampUsage(stamp, nowMs);

    return [usages = std::move(usages)](const std::string& normalizedUrl)
    {
        auto found = usages.find(normalizedUrl);
        return found != usages.end() ? found->second : 0.0;
    };
}

std::vector<GoItem> orderPalettePages(const PaletteRankingMaterials& materials,
                                      const std::string& query,
                                      std::int64_t nowMs)
{
    auto usage = makeUsageLookup(materials.stampsByNormalizedUrl, nowMs);
    auto rows = listPaletteRows(materials.tabs, materials.places, usage);
    auto ranked =
        rankPalette(rows, materials.history, {query, PaletteMode::goFromHere, usage});

    std::erase_if(ranked,
                  [](const GoItem& row)
                  { return row.kind == GoKind::search || row.kind == GoKind::typedURL; });
    return ranked;
}
} // namespace grazer
