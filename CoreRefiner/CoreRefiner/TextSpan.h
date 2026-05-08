#pragma once
#include <dwrite.h>
#include <optional>
#include <string>
#include "Colors.h"

class Color;

struct TextSpan
{
    UINT32 start = 0;
    UINT32 length = 0;

    std::optional<Color> color;
    std::optional<DWRITE_FONT_WEIGHT> weight;
    std::optional<std::wstring> fontFamily;
};