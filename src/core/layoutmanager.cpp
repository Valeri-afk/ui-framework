#include "ui_framework/core/linear_layout.hpp"
#include "ui_framework/core/stackpanelnode.hpp"
#include "layoutmanager.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

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

    float safeAdd(float a, float b) noexcept
    {
        if (!std::isfinite(a) || !std::isfinite(b))
            return kInfinity;

        const float result = a + b;

        return std::isfinite(result) ? result : kInfinity;
    }

    float safeSubtract(float a, float b) noexcept
    {
        if (!std::isfinite(a) || !std::isfinite(b))
            return 0.0f;

        return std::max(0.0f, a - b);
    }

    ui::LayoutSize sanitizeSize(ui::LayoutSize size) noexcept
    {
        return {
            finiteOrZero(size.width),
            finiteOrZero(size.height)};
    }

    ui::Padding sanitizePadding(ui::Padding padding) noexcept
    {
        padding.left = finiteOrZero(padding.left);
        padding.right = finiteOrZero(padding.right);
        padding.top = finiteOrZero(padding.top);
        padding.bottom = finiteOrZero(padding.bottom);

        return padding;
    }

    ui::Border sanitizeBorder(ui::Border border) noexcept
    {
        border.left = finiteOrZero(border.left);
        border.right = finiteOrZero(border.right);
        border.top = finiteOrZero(border.top);
        border.bottom = finiteOrZero(border.bottom);

        return border;
    }

    ui::LayoutConstraints normalizeConstraints(
        ui::LayoutConstraints constraints) noexcept
    {
        constraints.minWidth = finiteOrZero(constraints.minWidth);
        constraints.maxWidth = finiteOrInfinity(constraints.maxWidth);
        constraints.minHeight = finiteOrZero(constraints.minHeight);
        constraints.maxHeight = finiteOrInfinity(constraints.maxHeight);

        if (constraints.maxWidth < constraints.minWidth)
            constraints.maxWidth = constraints.minWidth;

        if (constraints.maxHeight < constraints.minHeight)
            constraints.maxHeight = constraints.minHeight;

        return constraints;
    }

    float subtractFromMax(float max, float value) noexcept
    {
        if (max >= kInfinity / 2.0f)
            return max;

        return safeSubtract(max, value);
    }

    ui::LayoutSize paddingBorderSize(
        const ui::Padding &padding,
        const ui::Border &border) noexcept
    {
        const float horizontal =
            safeAdd(
                safeAdd(padding.left, padding.right),
                safeAdd(border.left, border.right));

        const float vertical =
            safeAdd(
                safeAdd(padding.top, padding.bottom),
                safeAdd(border.top, border.bottom));

        return {
            horizontal,
            vertical};
    }

    ui::LayoutSize addPaddingBorder(
        ui::LayoutSize contentSize,
        const ui::Padding &padding,
        const ui::Border &border) noexcept
    {
        const ui::LayoutSize insets =
            paddingBorderSize(padding, border);

        return {
            safeAdd(contentSize.width, insets.width),
            safeAdd(contentSize.height, insets.height)};
    }

    ui::LayoutSize subtractPaddingBorder(
        ui::LayoutSize borderBoxSize,
        const ui::Padding &padding,
        const ui::Border &border) noexcept
    {
        const ui::LayoutSize insets =
            paddingBorderSize(padding, border);

        return {
            safeSubtract(borderBoxSize.width, insets.width),
            safeSubtract(borderBoxSize.height, insets.height)};
    }

    ui::LayoutPosition getContentPosition(const ui::Node &node) noexcept
    {
        const ui::LayoutPosition position = node.getActualPosition();

        const ui::Padding padding = sanitizePadding(node.getPadding());
        const ui::Border border = sanitizeBorder(node.getBorder());

        return {
            finiteOrZero(position.x) + border.left + padding.left,
            finiteOrZero(position.y) + border.top + padding.top};
    }

    ui::LayoutSize getContentSize(const ui::Node &node) noexcept
    {
        const ui::LayoutSize size = node.getActualSize();

        const ui::Padding padding = sanitizePadding(node.getPadding());
        const ui::Border border = sanitizeBorder(node.getBorder());

        const ui::LayoutSize insets =
            paddingBorderSize(padding, border);

        return {
            safeSubtract(finiteOrZero(size.width), insets.width),
            safeSubtract(finiteOrZero(size.height), insets.height)};
    }

    bool getRenderOutputSize(
        SDL_Renderer *renderer,
        int *width,
        int *height)
    {
        return SDL_GetCurrentRenderOutputSize(renderer, width, height) == 0;
    }

}

namespace ui
{
    LayoutManager::LayoutManager() = default;

    void LayoutManager::setViewportSize(
        const LayoutSize &size) noexcept
    {
        viewportSize_ = sanitizeSize(size);
    }

    LayoutSize LayoutManager::getViewportSize() const noexcept
    {
        return viewportSize_;
    }

    bool LayoutManager::syncViewportFromRenderer(
        SDL_Renderer *renderer)
    {
        if (!renderer)
            return false;

        int width = 0;
        int height = 0;

        if (!getRenderOutputSize(renderer, &width, &height))
            return false;

        const LayoutSize newSize{
            static_cast<float>(width),
            static_cast<float>(height)};

        if (newSize == viewportSize_)
            return false;

        viewportSize_ = sanitizeSize(newSize);
        return true;
    }

    void LayoutManager::requestFullLayout(
        NodeTree &nodeTree)
    {
        nodeTree.requestFullLayout();
    }

    void LayoutManager::processLayoutQueue(
        NodeTree &nodeTree)
    {
        {
            NodeTree::ScopedMutationGuard guard(nodeTree);

            nodeTree.forEachLayoutQueue(
                [this, &nodeTree](Node &root)
                {
                    if (!root.isVisible())
                        return;

                    const Node::Id rootId =
                        root.id();

                    const LayoutSize rootAvailable =
                        makeRootAvailableSize(root);

                    measureRecursive(
                        root,
                        rootAvailable,
                        nodeTree);

                    Node *liveRoot =
                        nodeTree.findNode(rootId);

                    if (!liveRoot)
                        return;

                    liveRoot->actualSize_ =
                        liveRoot->clampSize(
                            rootAvailable,
                            liveRoot->minSize_,
                            liveRoot->maxSize_);

                    liveRoot->actualPosition_ =
                        liveRoot->position_;

                    arrangeRecursive(
                        *liveRoot,
                        nodeTree);
                });
        }

        nodeTree.flushMutationQueue();
    }

    void LayoutManager::measureRecursive(
        Node &node,
        const LayoutSize &availableBorderBoxSize,
        NodeTree &nodeTree)
    {
        if (!node.isVisible())
            return;

        const Node::Id nodeId = node.id();

        LayoutSize availableBorder = sanitizeSize(availableBorderBoxSize);

        // Fixed size ограничивает measure.
        if (node.size_.width.isValue())
            availableBorder.width = node.size_.width.value;

        if (node.size_.height.isValue())
            availableBorder.height = node.size_.height.value;

        const LayoutSize availableContent =
            toContentSize(node, availableBorder);

        MeasureContext ctx;
        ctx.availableSize = availableContent;

        ctx.measureChild =
            [this, &nodeTree, &node](
                size_t visibleChildIndex,
                const LayoutSize &childAvailableContent) -> LayoutSize
        {
            Node *child = node.getVisibleChild(visibleChildIndex);

            if (!child)
                return {};

            const Node::Id childId = child->id();

            const LayoutSize childAvailableBorder =
                toBorderBoxSize(*child, childAvailableContent);

            measureRecursive(
                *child,
                childAvailableBorder,
                nodeTree);

            Node *liveChild = nodeTree.findNode(childId);

            return liveChild
                       ? liveChild->desiredSize_
                       : LayoutSize{};
        };

        LayoutSize desiredContent{};
        
        if (auto *stackPanel = dynamic_cast<StackPanelNode *>(&node))
        {
            desiredContent =
                sanitizeSize(
                    internal::measureLinearPanel(
                        *stackPanel,
                        ctx));
        }
        else
        {
            desiredContent =
                sanitizeSize(
                    node.measure(ctx));
        }

        Node *liveNode = nodeTree.findNode(nodeId);

        if (!liveNode)
            return;

        LayoutSize desiredBorder =
            toBorderBoxSize(*liveNode, desiredContent);

        desiredBorder =
            applyMeasureRules(*liveNode, desiredBorder);

        liveNode->desiredSize_ = desiredBorder;
    }

    void LayoutManager::arrangeRecursive(
        Node &node,
        NodeTree &nodeTree)
    {
        if (!node.isVisible())
            return;

        ArrangeContext ctx;

        ctx.contentPosition = getContentPosition(node);
        ctx.contentSize = getContentSize(node);

        ctx.placeChild =
            [this, &nodeTree, &node](
                size_t visibleChildIndex,
                const LayoutPosition &position,
                const LayoutSize &size)
        {
            Node *child = node.getVisibleChild(visibleChildIndex);

            if (!child || !child->isVisible())
                return;

            LayoutSize finalSize =
                child->clampSize(
                    sanitizeSize(size),
                    child->getMinSize(),
                    child->getMaxSize());

            child->actualPosition_ = position;
            child->actualSize_ = finalSize;

            arrangeRecursive(*child, nodeTree);
        };

        if (auto *stackPanel = dynamic_cast<StackPanelNode *>(&node))
        {
            internal::arrangeLinearPanel(
                *stackPanel,
                ctx);
        }
        else
        {
            node.arrange(ctx);
        }
    }

    LayoutSize LayoutManager::makeRootAvailableSize(
        const Node &root) const
    {
        LayoutSize size = viewportSize_;

        if (root.size_.width.isValue())
            size.width = root.size_.width.value;

        if (root.size_.height.isValue())
            size.height = root.size_.height.value;

        return sanitizeSize(size);
    }

    LayoutSize LayoutManager::toContentSize(
        const Node &node,
        const LayoutSize &borderBoxSize) const
    {
        return subtractPaddingBorder(
            sanitizeSize(borderBoxSize),
            sanitizePadding(node.getPadding()),
            sanitizeBorder(node.getBorder()));
    }

    LayoutSize LayoutManager::toBorderBoxSize(
        const Node &node,
        const LayoutSize &contentSize) const
    {
        return addPaddingBorder(
            sanitizeSize(contentSize),
            sanitizePadding(node.getPadding()),
            sanitizeBorder(node.getBorder()));
    }

    LayoutSize LayoutManager::applyMeasureRules(
        const Node &node,
        const LayoutSize &measuredBorderBoxSize) const
    {
        LayoutSize result = sanitizeSize(measuredBorderBoxSize);

        if (node.size_.width.isValue())
            result.width = node.size_.width.value;

        if (node.size_.height.isValue())
            result.height = node.size_.height.value;

        result =
            node.clampSize(
                result,
                node.getMinSize(),
                node.getMaxSize());

        return sanitizeSize(result);
    }

    void LayoutManager::sanitizeLayoutSize(
        LayoutSize &size) const noexcept
    {
        size.width = finiteOrZero(size.width);
        size.height = finiteOrZero(size.height);
    }

}
