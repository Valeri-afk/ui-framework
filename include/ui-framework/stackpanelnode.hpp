#pragma once

#include "node.hpp"
#include "panelnode.hpp"
#include "ui_framework/types.hpp"

namespace ui
{

    class StackPanelNode : public PanelNode
    {
    public:
        enum class Orientation
        {
            Vertical,
            Horizontal
        };

        explicit StackPanelNode(
            Orientation orientation = Orientation::Vertical);

        void setOrientation(Orientation orientation);
        Orientation getOrientation() const noexcept;

    protected:
        LayoutSize measure(MeasureContext &ctx) override;
        void arrange(ArrangeContext &ctx) override;

    private:
        Orientation orientation_ = Orientation::Vertical;
    };

}
