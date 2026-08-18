#pragma once

#include "ui_framework/types.hpp"

namespace ui
{

    class StackPanelNode;

    namespace internal
    {
        LayoutSize measureLinearPanel(
            StackPanelNode &panel,
            MeasureContext &ctx);

        void arrangeLinearPanel(
            StackPanelNode &panel,
            ArrangeContext &ctx);
    }

}
