#pragma once

#include "ui_framework/types.hpp"

#include <cstddef>
#include <functional>

namespace ui::internal
{

    struct MeasureContext
    {
        LayoutSize availableSize;

        std::function<LayoutSize(
            size_t,
            const LayoutSize &)> measureChild;
    };

    struct ArrangeContext
    {
        LayoutPosition contentPosition;
        LayoutSize contentSize;

        std::function<void(
            size_t,
            const LayoutPosition &,
            const LayoutSize &)> placeChild;
    };

}
