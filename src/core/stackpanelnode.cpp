#include "ui_framework/core/stackpanelnode.hpp"
#include "ui_framework/core/linear_layout.hpp"

namespace ui
{

    StackPanelNode::StackPanelNode(Orientation orientation)
        : orientation_(orientation)
    {
    }

    void StackPanelNode::setOrientation(Orientation orientation)
    {
        if (orientation_ == orientation)
            return;

        deferLayoutMutation(
            [orientation](Node &node)
            {
                static_cast<StackPanelNode &>(node).orientation_ =
                    orientation;
            });
    }

    StackPanelNode::Orientation StackPanelNode::getOrientation()
        const noexcept
    {
        return orientation_;
    }

    LayoutSize StackPanelNode::measure(MeasureContext &ctx)
    {
        return internal::measureLinearPanel(*this, ctx);
    }

    void StackPanelNode::arrange(ArrangeContext &ctx)
    {
        internal::arrangeLinearPanel(*this, ctx);
    }

}
