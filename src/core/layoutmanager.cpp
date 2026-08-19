#include "ui_framework/core/linear_layout.hpp"
#include "ui_framework/core/layout_constraints.hpp"
#include "ui_framework/core/stackpanelnode.hpp"
#include "ui_framework/core/textnode.hpp"
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
        if (!std::isfinite(a))
            return kInfinity;

        if (!std::isfinite(b))
            return 0.0f;

        return std::max(0.0f, a - b);
    }

    ui::LayoutSize sanitizeSize(ui::LayoutSize size) noexcept
    {
        return {
            finiteOrZero(size.width),
            finiteOrZero(size.height)};
    }

    ui::LayoutSize sanitizeProposal(ui::LayoutSize size) noexcept
    {
        return {
            finiteOrInfinity(size.width),
            finiteOrInfinity(size.height)};
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

    float subtractInset(float value, float inset) noexcept
    {
        if (value >= kInfinity / 2.0f)
            return kInfinity;

        return safeSubtract(value, inset);
    }

    ui::LayoutSize subtractPaddingBorder(
        ui::LayoutSize borderBoxSize,
        const ui::Padding &padding,
        const ui::Border &border) noexcept
    {
        const ui::LayoutSize insets =
            paddingBorderSize(padding, border);

        return {
            subtractInset(borderBoxSize.width, insets.width),
            subtractInset(borderBoxSize.height, insets.height)};
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
            std::max(
                0.0f,
                finiteOrZero(size.width) - insets.width),
            std::max(
                0.0f,
                finiteOrZero(size.height) - insets.height)};
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
                        internal::resolveFinalSize(
                            *liveRoot,
                            rootAvailable);

                    liveRoot->actualPosition_ =
                        liveRoot->position_;

                    arrangeRecursive(
                        *liveRoot,
                        nodeTree);
                });
        }

        nodeTree.flushMutationQueue();
    }

    LayoutSize LayoutManager::measureTextNode(
        TextNode &node,
        const LayoutSize &availableContent) const
    {
        if (!node.font_ || node.text_.empty())
            return {};

        const float availableWidth =
            finiteOrInfinity(availableContent.width);

        int width = 0;
        int height = 0;

        const bool measured =
            availableWidth < kInfinity
                ? TTF_GetStringSizeWrapped(
                      node.font_,
                      node.text_.c_str(),
                      node.text_.size(),
                      static_cast<int>(std::round(
                          std::max(0.0f, availableWidth))),
                      &width,
                      &height)
                : TTF_GetStringSize(
                      node.font_,
                      node.text_.c_str(),
                      node.text_.size(),
                      &width,
                      &height);

        if (!measured)
            return {};

        return {
            static_cast<float>(width),
            static_cast<float>(height)};
    }

    void LayoutManager::measureRecursive(
        Node &node,
        const LayoutSize &availableBorderBoxSize,
        NodeTree &nodeTree)
    {
        if (!node.isVisible())
            return;

        const Node::Id nodeId = node.id();

        LayoutSize availableBorder =
            sanitizeProposal(availableBorderBoxSize);

        availableBorder =
            internal::resolveMeasurementProposal(
                node,
                availableBorder);

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

            for (size_t i = 0; i < stackPanel->childCount(); ++i)
            {
                Node *child = stackPanel->getChildAt(i);

                if (!child ||
                    !child->isVisible() ||
                    child->getPositionMode() != PositionMode::Absolute)
                {
                    continue;
                }

                const LayoutSize childAvailableBorder =
                    toBorderBoxSize(*child, availableContent);

                measureRecursive(
                    *child,
                    childAvailableBorder,
                    nodeTree);
            }
        }
        else if (auto *textNode = dynamic_cast<TextNode *>(&node))
        {
            desiredContent =
                sanitizeSize(
                    measureTextNode(
                        *textNode,
                        availableContent));
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
            internal::resolveFinalSize(
                *liveNode,
                desiredBorder);

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
                internal::resolveFinalSize(
                    *child,
                    sanitizeSize(size));

            child->actualPosition_ = position;
            child->actualSize_ = finalSize;

            arrangeRecursive(*child, nodeTree);
        };

        if (auto *stackPanel = dynamic_cast<StackPanelNode *>(&node))
        {
            internal::arrangeLinearPanel(
                *stackPanel,
                ctx);

            for (size_t i = 0; i < stackPanel->childCount(); ++i)
            {
                Node *child = stackPanel->getChildAt(i);

                if (!child ||
                    !child->isVisible() ||
                    child->getPositionMode() != PositionMode::Absolute)
                {
                    continue;
                }

                LayoutSize finalSize =
                    internal::resolveFinalSize(
                        *child,
                        sanitizeSize(child->desiredSize_));

                child->actualPosition_ =
                    ctx.contentPosition + child->position_;
                child->actualSize_ = finalSize;

                arrangeRecursive(*child, nodeTree);
            }
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
}