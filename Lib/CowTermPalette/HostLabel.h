#pragma once

#include <string>

namespace term::remote
{
// How a host is shown: "tamby-windows" for "jamie@tamby-windows".
std::string getHostLabel(const std::string& host);
} // namespace term::remote
