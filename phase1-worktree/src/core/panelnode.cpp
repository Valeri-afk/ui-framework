#include "ui_framework/core/panelnode.hpp"

#include "nodetree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace
{

    float finiteOrZero(float value) noexcept
    {
        return std::isfinite(value) ? value : 0.0f;
    }

    ui::LayoutSize sanitizeSize(ui::LayoutSize size) noexcept
    {
        return {
            finiteOrZero(size.width),
            finiteOrZero(size.height)};
    }