#include <CowTermPalette/RankingMaterials.h>

#include <FrecencySearch/LearnedPicks.h>
#include <FrecencySearch/MatchLevel.h>
#include <Miro/Reflection/Serialize.h>
#include <NanoTest/NanoTest.h>

#include <string>
#include <vector>

using namespace nano;
using namespace term;

// Rudder orders CowTerm's rows from the materials CowTerm hands it; these
// tests hold that order to the one CowTerm's own palette gives the same inputs.

namespace
{
constexpr std::int64_t nowMs = 1'790'000'000'000LL;
constexpr std::int64_t minuteMs = 60LL * 1000;

PaletteItem makeItem(PaletteItem::Kind kind, const std::string& label, const std::string& key)
{
    auto item = PaletteItem {};
    item.kind = kind;
    item.key = key;
    item.label = label;
    item.detail = key;
    return item;
}

// Sessions as the palette builds them, then projects not open, then a remote
// project and an unreachable host: a Claude session with an unread mark, two
// sessions used at the same moment (a tie), a project and a session sharing a
// name's start, and a pick learned for "gr".
PaletteRankingMaterials makeWorkdayMaterials()
{
    auto materials = PaletteRankingMaterials {};

    auto claude = makeItem(PaletteItem::Kind::Session, "Rudder", "/p/Rudder");
    claude.claude = true;
    claude.lastUsed = nowMs - 5 * minuteMs;
    claude.unread = UnreadMark {"finished", nowMs - minuteMs, "Done."};

    auto grazer = makeItem(PaletteItem::Kind::Session, "Grazer", "/p/Grazer");
    grazer.lastUsed = nowMs - 30 * minuteMs;

    auto grader = makeItem(PaletteItem::Kind::Session, "grader", "/p/grader");
    grader.lastUsed = nowMs - 30 * minuteMs;
    grader.status = "build finished";

    auto worktree = makeItem(PaletteItem::Kind::Project, "Grazer worktree", "/p/Grazer-wt");
    worktree.lastUsed = nowMs - 600 * minuteMs;

    auto analyzer = makeItem(PaletteItem::Kind::Project, "graphics-analyzer", "/p/graphics-analyzer");

    auto remote = makeItem(PaletteItem::Kind::Project, "miro", "macprorack:/home/j/miro");
    remote.host = "jamie@macprorack";

    auto unreachable = makeItem(PaletteItem::Kind::Project, "tamby — unreachable", "");
    unreachable.host = "tamby";
    unreachable.unavailable = true;

    materials.items = {claude, grazer, grader, worktree, analyzer, remote, unreachable};
    materials.usageByKey = {{"/p/Rudder", {4, nowMs - 60 * minuteMs}},
                            {"/p/Grazer", {2, nowMs - 60 * minuteMs}},
                            {"/p/grader", {2, nowMs - 60 * minuteMs}},
                            {"/p/Grazer-wt", {1, nowMs - 900 * minuteMs}}};
    materials.learnedPicks = {{"gr", "/p/Grazer-wt", 3, nowMs - 10 * minuteMs},
                              {"gra", "/p/graphics-analyzer", 1, nowMs - 20 * minuteMs}};
    return materials;
}

// What CowTerm's palette shows for `query`: orderPaletteItems with
// PaletteUsageStore's usage (each stored count decayed to now) and its
// learned picks for the text.
std::vector<PaletteItem> orderAsCowTermsPaletteDoes(const PaletteRankingMaterials& materials,
                                                    const std::string& query)
{
    auto usage = [&materials](const std::string& key)
    {
        auto found = materials.usageByKey.find(key);

        if (found == materials.usageByKey.end())
            return 0.0;

        return frecencysearch::calculateDecayedUsage(found->second.count, found->second.at, nowMs);
    };

    auto picks = std::vector<frecencysearch::LearnedPick> {};

    for (auto& entry: materials.learnedPicks)
        picks.push_back({entry.query, entry.destinationKey, entry.count, entry.at});

    auto learned =
        frecencysearch::findLearnedPicksForQuery(picks, frecencysearch::normaliseQuery(query), nowMs);
    return orderPaletteItems(materials.items, query, {usage, learned});
}

std::vector<std::string> describeRows(const std::vector<PaletteItem>& rows)
{
    auto described = std::vector<std::string> {};

    for (auto& row: rows)
        described.push_back(row.host + " " + row.key + " " + row.label);

    return described;
}

const std::vector<std::string> typedTexts {
    "", "g", "gr", "gra", "graz", "grazer", "rudder", "claude", "finished", "miro",
    "macprorackmiro", "wt", "tamby", "p", "zzz"};
} // namespace

auto tCowTermSameOrder = test("CowTermPaletteMaterials/the rows come in CowTerm's palette order for every typed text") = []
{
    auto materials = makeWorkdayMaterials();

    for (auto& typed: typedTexts)
        check(describeRows(orderPaletteItemsFromMaterials(materials, typed, nowMs))
                  == describeRows(orderAsCowTermsPaletteDoes(materials, typed)),
              "typed: " + typed);
};

auto tCowTermMaterialsSurviveJson = test("CowTermPaletteMaterials/the materials keep the order through JSON") = []
{
    auto materials = makeWorkdayMaterials();
    auto received =
        Miro::createFromJSONString<PaletteRankingMaterials>(Miro::toJSONString(materials));

    for (auto& typed: typedTexts)
        check(describeRows(orderPaletteItemsFromMaterials(received, typed, nowMs))
                  == describeRows(orderAsCowTermsPaletteDoes(materials, typed)),
              "typed: " + typed);
};

auto tCowTermAwkwardCases = test("CowTermPaletteMaterials/unread leads with nothing typed, a learned pick leads its text") = []
{
    auto materials = makeWorkdayMaterials();

    auto empty = orderPaletteItemsFromMaterials(materials, "", nowMs);
    check(empty.size() == materials.items.size());
    check(empty.front().key == "/p/Rudder");
    check(empty[1].key == "/p/Grazer" && empty[2].key == "/p/grader", "a tie keeps the palette's order");

    auto learned = orderPaletteItemsFromMaterials(materials, "gr", nowMs);
    check(!learned.empty() && learned.front().key == "/p/Grazer-wt");
};
