#pragma once

#include "UnreadMark.h"

#include <Miro/Reflect.h>

#include <cstdint>
#include <optional>
#include <string>

namespace term
{
class TermSession;

struct PaletteItem
{
    enum class Kind
    {
        Session,
        Project
    };

    Kind kind = Kind::Project;
    std::string key;
    std::string label;
    std::string detail;
    std::string status;
    bool claude = false;

    // The remote machine the row lives on; empty for this one.
    std::string host;

    // A remote host that can't be reached: shown, greyed, never opened.
    bool unavailable = false;
    std::int64_t lastUsed = 0;

    // An open session with a pane Jamie has not seen.
    std::optional<UnreadMark> unread;
    TermSession* session = nullptr;

    // Everything but `session`, which points into the app that built the row.
    MIRO_REFLECT(kind, key, label, detail, status, claude, host, unavailable, lastUsed, unread)
};
} // namespace term
