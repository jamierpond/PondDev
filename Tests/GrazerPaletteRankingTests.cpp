#include <GrazerPalette/RankingMaterials.h>

#include <FrecencySearch/LearnedPicks.h>
#include <Miro/Reflection/Serialize.h>
#include <NanoTest/NanoTest.h>

#include <string>
#include <vector>

using namespace nano;
using namespace grazer;

// Rudder orders Grazer's pages from the materials Grazer hands it; these tests
// hold that order to the one Grazer's own palette gives the same inputs.

namespace
{
constexpr std::int64_t nowMs = 1'790'000'000'000LL;
constexpr std::int64_t hourMs = 60LL * 60 * 1000;

PaletteTab makeTab(std::int64_t tabId, const std::string& title, const std::string& url)
{
    auto tab = PaletteTab {};
    tab.tabId = tabId;
    tab.title = title;
    tab.url = url;
    return tab;
}

// Two tabs on GitHub viewed at the same moment (a tie), a failed tab, the tab
// the palette opened over, a starred place that isn't open, history with a
// page used lately that has no tab, and a past search.
PaletteRankingMaterials makeWorkdayMaterials()
{
    auto materials = PaletteRankingMaterials {};

    auto pulls = makeTab(1, "Pull requests", "https://github.com/pulls");
    pulls.lastViewedAtMs = nowMs - hourMs;
    auto issues = makeTab(2, "Issues", "https://github.com/issues");
    issues.lastViewedAtMs = nowMs - hourMs;
    auto failed = makeTab(3, "github.com/broken", "https://github.com/broken");
    failed.failed = true;
    failed.lastViewedAtMs = nowMs - 2 * hourMs;
    auto current = makeTab(4, "GitHub", "https://github.com/");
    current.isCurrent = true;
    current.lastViewedAtMs = nowMs;
    materials.tabs = {pulls, issues, failed, current};

    materials.places = {{"GitHub Actions", "https://github.com/actions"},
                        {"Hacker News", "https://news.ycombinator.com/"},
                        {"Issues", "https://github.com/issues"}};

    materials.stampsByNormalizedUrl = {
        {"github.com/pulls", {nowMs - hourMs, 3}},
        {"github.com/issues", {nowMs - hourMs, 3}},
        {"github.com/actions", {nowMs - 24 * hourMs, 1}},
        {"github.com/notifications", {nowMs - 2 * hourMs, 5}},
        {"news.ycombinator.com", {nowMs - 48 * hourMs, 2}}};

    materials.history = {
        {"https://github.com/notifications", "Notifications", nowMs - 2 * hourMs, 9},
        {"https://github.com/settings", "Settings", nowMs - 72 * hourMs, 1},
        {"https://www.google.com/search?q=github", "github - Google Search", nowMs - hourMs, 1},
        {"https://github.com/pulls", "Pull requests", nowMs - hourMs, 30}};

    return materials;
}

// What Grazer's palette shows for `query`: listPaletteRows when it opens,
// rankPalette on each keystroke, with each page's MruStore usage decayed to
// now -- less the rows that act on the typed text itself.
std::vector<GoItem> orderAsGrazersPaletteDoes(const PaletteRankingMaterials& materials,
                                              const std::string& query)
{
    auto usage = [&materials](const std::string& normalizedUrl)
    {
        auto found = materials.stampsByNormalizedUrl.find(normalizedUrl);

        if (found == materials.stampsByNormalizedUrl.end())
            return 0.0;

        return frecencysearch::calculateDecayedUsage(found->second.usage, found->second.at, nowMs);
    };

    auto rows = listPaletteRows(materials.tabs, materials.places, usage);
    auto shown = rankPalette(rows, materials.history, {query, PaletteMode::newTab, usage});
    auto pages = std::vector<GoItem> {};

    for (auto& row: shown)
        if (row.kind != GoKind::search && row.kind != GoKind::typedURL)
            pages.push_back(row);

    return pages;
}

std::vector<std::string> describeRows(const std::vector<GoItem>& rows)
{
    auto described = std::vector<std::string> {};

    for (auto& row: rows)
        described.push_back(row.kind + " " + std::to_string(row.tabId) + " " + row.url);

    return described;
}

const std::vector<std::string> typedTexts {
    "", "g", "git", "github", "issues", "pull", "hub", "news", "notif", "set",
    "broken", "github.com/issues", "https://github.com/pulls", "ycombinator.com", "zzz"};
} // namespace

auto tSameOrderAsGrazer = test("GrazerPaletteRanking/the pages come in Grazer's palette order for every typed text") = []
{
    auto materials = makeWorkdayMaterials();

    for (auto& typed: typedTexts)
        check(describeRows(orderPalettePages(materials, typed, nowMs))
                  == describeRows(orderAsGrazersPaletteDoes(materials, typed)),
              "typed: " + typed);
};

auto tGrazerMaterialsSurviveJson = test("GrazerPaletteRanking/the materials keep the order through JSON") = []
{
    auto materials = makeWorkdayMaterials();
    auto received =
        Miro::createFromJSONString<PaletteRankingMaterials>(Miro::toJSONString(materials));

    for (auto& typed: typedTexts)
        check(describeRows(orderPalettePages(received, typed, nowMs))
                  == describeRows(orderAsGrazersPaletteDoes(materials, typed)),
              "typed: " + typed);
};

auto tGrazerAwkwardCases = test("GrazerPaletteRanking/a used page with no tab is listed, a failed page and the current tab sink") = []
{
    auto materials = makeWorkdayMaterials();
    auto rows = orderPalettePages(materials, "github", nowMs);

    check(!rows.empty());
    check(rows.back().isCurrent);
    check(rows[rows.size() - 2].failed);

    auto notifications = std::find_if(rows.begin(),
                                      rows.end(),
                                      [](const GoItem& row)
                                      { return row.url == "https://github.com/notifications"; });
    check(notifications != rows.end() && notifications->kind == GoKind::history);

    auto empty = orderPalettePages(materials, "", nowMs);
    check(empty.size() == 6);
    check(empty.front().tabId == 1 && empty[1].tabId == 2);
    check(empty.back().isCurrent);
};
