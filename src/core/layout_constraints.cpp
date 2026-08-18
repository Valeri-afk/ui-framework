#include "ui_framework/core/layout_constraints.hpp"

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

    float finiteOrInfinity(float value) noexcept
    {
        return std::isfinite(value) ? value : kInfinity;
    }

    float proposalBoundedByMax(float proposal, float maxValue) noexcept
    {
        return std::min(finiteOrInfinity(proposal), finiteOrInfinity(maxValue));
    }
}

namespace ui::internal
{

    LayoutSize resolveMeasurementProposal(const Node &node, LayoutSize proposal) noexcept
    {
        proposal.width = finiteOrInfinity(proposal.width);
        proposal.height = finiteOrInfinity(proposal.height);

        const LayoutSizeValue size = node.getSize();
        const LayoutSize maxSize = node.getMaxSize();

        if (size.width.isValue())
            proposal.width = finiteOrZero(size.width.value);
        else
            proposal.width = proposalBoundedByMax(proposal.width, maxSize.width);

        if (size.height.isValue())
            proposal.height = finiteOrZero(size.height.value);
        else
            proposal.height = proposalBoundedByMax(proposal.height, maxSize.height);

        return proposal;
    }

    LayoutSize resolveFinalSize(const Node &node, LayoutSize allocated) noexcept
    {
        allocated.width = finiteOrZero(allocated.width);
        allocated.height = finiteOrZero(allocated.height);

        const LayoutSizeValue size = node.getSize();
        const LayoutSize minSize = node.getMinSize();
        const LayoutSize maxSize = node.getMaxSize();

        if (size.width.isValue())
            allocated.width = finiteOrZero(size.width.value);
        else
        {
            const float minWidth = finiteOrZero(minSize.width);
            const float maxWidth = finiteOrInfinity(maxSize.width);
            allocated.width = std::clamp(allocated.width, minWidth, std::max(minWidth, maxWidth));
        }

        if (size.height.isValue())
            allocated.height = finiteOrZero(size.height.value);
        else
        {
            const float minHeight = finiteOrZero(minSize.height);
            const float maxHeight = finiteOrInfinity(maxSize.height);
            allocated.height = std::clamp(allocated.height, minHeight, std::max(minHeight, maxHeight));
        }

        return allocated;
    }

}
