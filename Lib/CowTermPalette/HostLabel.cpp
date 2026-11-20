#include "HostLabel.h"

namespace term::remote
{
std::string getHostLabel(const std::string& host)
{
    const auto at = host.find('@');

    if (at == std::string::npos)
        return host;

    return host.substr(at + 1);
}
} // namespace term::remote
