#include "ui_framework/core/stackpanelnode.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{

    constexpr float kInfinity = std::numeric_limits<float>::max();

    float finiteOrZero(float value) noexcept
    {
        return std::isfinite(value) ? value : 0.0f;
    }

    float safeAdd(float a, float b) noexcept
    {
        if (!std::isfinite(a) || !std::isfinite(b))
            return kInfinity;

        const float result = a + b;

        return std::isfinite(result) ? result : kInfinity;
    }

    bool isVerticalOrientation(
        ui::StackPanelNode::Orientation orientation) noexcept
    {
        return orientation == ui::StackPanelNode::Orientation::Vertical;
    }

    void applyChildConstraints(
        ui::LayoutConstraints &constraints,
        bool vertical) noexcept
    {
        if (vertical)
        {
            constraints.minHeight = 0.0f;
            constraints.maxHeight = kInfinity;
        }
        else
        {
            constraints.minWidth = 0.0f;
            constraints.maxWidth = kInfinity;
        }
    }

    void accumulateContentSize(
        ui::LayoutSize &contentSize,
        ui::LayoutSize childSize,
        bool vertical) noexcept
    {
        if (vertical)
        {
            contentSize.width =
                std::max(
                    finiteOrZero(contentSize.width),
                    finiteOrZero(childSize.width));

            contentSize.height =
                safeAdd(
                    finiteOrZero(contentSize.height),
                    finiteOrZero(childSize.height));
        }
        else
        {
            contentSize.width =
                safeAdd(
                    finiteOrZero(contentSize.width),
                    finiteOrZero(childSize.width));

            contentSize.height =
                std::max(
                    finiteOrZero(contentSize.height),
                    finiteOrZero(childSize.height));
        }
    }

    void advanceArrangePosition(
        ui::LayoutPosition &position,
        ui::LayoutSize appliedSize,
        bool vertical) noexcept
    {
        if (vertical)
        {
            position.y += finiteOrZero(appliedSize.height);
        }
        else
        {
            position.x += finiteOrZero(appliedSize.width);
        }
    }

}

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
        if (!ctx.measureChild)
            return {};

        const bool vertical = isVerticalOrientation(orientation_);

        LayoutSize contentSize{};
        size_t visibleIndex = 0;

        for (size_t i = 0; i < childCount(); ++i)
        {
            Node *child = getChildAt(i);

            if (!child || !child->isVisible())
                continue;

            LayoutSize childAvailable = ctx.availableSize;

            if (vertical)
                childAvailable.height = kInfinity;
            else
                childAvailable.width = kInfinity;

            const LayoutSize childSize =
                ctx.measureChild(visibleIndex, childAvailable);

            accumulateContentSize(contentSize, childSize, vertical);

            ++visibleIndex;
        }

        return contentSize;
    }

    void StackPanelNode::arrange(ArrangeContext &ctx)
    {
        if (!ctx.placeChild)
            return;

        const bool vertical =
            isVerticalOrientation(orientation_);

        LayoutPosition position = ctx.contentPosition;

        size_t visibleIndex = 0;

        for (size_t i = 0; i < childCount(); ++i)
        {
            Node *child = getChildAt(i);

            if (!child || !child->isVisible())
                continue;

            const LayoutSize desired = child->getDesiredSize();

            LayoutSize finalSize = desired;

            if (vertical)
                finalSize.width = ctx.contentSize.width;
            else
                finalSize.height = ctx.contentSize.height;

            ctx.placeChild(
                visibleIndex,
                position,
                finalSize);

            if (vertical)
                position.y += finalSize.height;
            else
                position.x += finalSize.width;

            ++visibleIndex;
        }
    }
}
