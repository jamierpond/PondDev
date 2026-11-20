#pragma once

#include "PaletteItem.h"
#include "PaletteRanking.h"
#include "PaletteUsageEntries.h"

#include <Miro/Reflect.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace term
{
// Every input CowTerm's palette ordering reads, as CowTerm has it: the rows
// its palette builds (sessions, then projects not open, then remote ones),
// each session's and project's stored use count by key, and every learned
// pick. What CowTerm hands Rudder when asked, so Rudder orders CowTerm's rows
// exactly as CowTerm's palette does.
struct PaletteRankingMaterials
{
    std::vector<PaletteItem> items;
    std::map<std::string, UsageEntry> usageByKey;
    std::vector<LearnedPickEntry> learnedPicks;

    MIRO_REFLECT(items, usageByKey, learnedPicks)
};

// The name the materials go by in Rudder's protocol.
constexpr auto paletteRankingMaterialsName = "cowterm-palette";

// Each key's usage decayed to `nowMs`, and the picks learned for `query`.
PaletteUsage makePaletteUsage(const std::map<std::string, UsageEntry>& usageByKey,
                              const std::vector<LearnedPickEntry>& learnedPicks,
                              const std::string& query,
                              std::int64_t nowMs);

// The rows CowTerm's palette shows for `query`, in its order.
std::vector<PaletteItem> orderPaletteItemsFromMaterials(const PaletteRankingMaterials& materials,
                                                        const std::string& query,
                                                        std::int64_t nowMs);
} // namespace term
