#pragma once

#include <Miro/Reflect.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace term
{
struct UnreadMark
{
    std::string event;
    std::int64_t sinceMilliseconds = 0;
    std::string text;

    MIRO_REFLECT(event, sinceMilliseconds, text)
};

// Unread rows first, the newest first; the rest keep their order. readMark
// gives a row's mark, empty when it is read.
template <typename Row, typename ReadMark>
void moveUnreadRowsFirst(std::vector<Row>& rows, ReadMark readMark)
{
    const auto shouldComeFirst = [&readMark](const Row& first, const Row& second)
    {
        const auto& firstMark = readMark(first);
        const auto& secondMark = readMark(second);

        if (firstMark.has_value() != secondMark.has_value())
            return firstMark.has_value();

        if (!firstMark)
            return false;

        return firstMark->sinceMilliseconds > secondMark->sinceMilliseconds;
    };

    std::ranges::stable_sort(rows, shouldComeFirst);
}
} // namespace term
