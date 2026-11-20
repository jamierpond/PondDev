#pragma once

#include <Miro/Reflect.h>

#include <string>

namespace grazer
{
struct Place
{
    std::string title;
    std::string url;

    MIRO_REFLECT(title, url)
};
} // namespace grazer
