#pragma once

#include "ui_framework/types.hpp"
#include "ui_framework/components/base/widget.hpp"

namespace ui
{
    class Paper : public Widget
    {
    public:
        Paper();
        Paper(const LayoutPosition &position, const LayoutSize &size);
    };
}