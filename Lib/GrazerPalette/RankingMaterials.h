#pragma once

#include "HistoryEntry.h"
#include "Palette.h"
#include "Place.h"
#include "Stamp.h"
#include "Types.h"

#include <Miro/Reflect.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace grazer
{
// Every input Grazer's palette ordering reads, as Grazer recorded it: its
// open tabs, its starred places, its recency stamps by normalized URL, and
// its history in the order the store lists it. What Grazer hands Rudder when
// asked, so Rudder orders Grazer's pages exactly as Grazer's palette does.
struct PaletteRankingMaterials
{
    std::vector<PaletteTab> tabs;
    std::vector<Place> places;
    std::map<std::string, Stamp> stampsByNormalizedUrl;
    std::vector<HistoryEntry> history;

    MIRO_REFLECT(tabs, places, stampsByNormalizedUrl, history)
};

// The name the materials go by in Rudder's protocol.
constexpr auto paletteRankingMaterialsName = "grazer-palette";

// The stamp's uses decayed to `nowMs`.
double calculateStampUsage(const Stamp& stamp, std::int64_t nowMs);

// Each page's usage decayed to `nowMs`, by normalized URL; 0 for a page with
// no stamp.
UsageFn makeUsageLookup(const std::map<std::string, Stamp>& stampsByNormalizedUrl,
                        std::int64_t nowMs);

// The page rows Grazer's palette shows for `query`, in its order: tabs,
// places and history as listPaletteRows and rankPalette order them, without
// the rows that act on the typed text itself (search for it, open it).
std::vector<GoItem> orderPalettePages(const PaletteRankingMaterials& materials,
                                      const std::string& query,
                                      std::int64_t nowMs);
} // namespace grazer
