#include "ui_framework/core/stackpanelnode.hpp"
#include "ui_framework/core/linear_layout.hpp"

#include <cmath>

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

    void StackPanelNode::setGap(float gap)
    {
        if (!std::isfinite(gap) || gap < 0.0f)
            return;

        if (gap_ == gap)
            return;

        deferLayoutMutation(
            [gap](Node &node)
            {
                static_cast<StackPanelNode &>(node).gap_ = gap;
            });
    }

    float StackPanelNode::getGap() const noexcept
    {
        return gap_;
    }

    void StackPanelNode::setMainAlignment(Alignment alignment)
    {
        if (mainAlignment_ == alignment)
            return;

        deferLayoutMutation(
            [alignment](Node &node)
            {
                static_cast<StackPanelNode &>(node).mainAlignment_ =
                    alignment;
            });
    }

    Alignment StackPanelNode::getMainAlignment() const noexcept
    {
        return mainAlignment_;
    }

    void StackPanelNode::setCrossAlignment(Alignment alignment)
    {
        if (crossAlignment_ == alignment)
            return;

        deferLayoutMutation(
            [alignment](Node &node)
            {
                static_cast<StackPanelNode &>(node).crossAlignment_ =
                    alignment;
            });
    }

    Alignment StackPanelNode::getCrossAlignment() const noexcept
    {
        return crossAlignment_;
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
