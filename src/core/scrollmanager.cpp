#include "scrollmanager.hpp"

#include <algorithm>

namespace ui
{
    ScrollOffset ScrollState::maxOffset() const noexcept
    {
        return {
            std::max(0.0f, content.width - viewport.width),
            std::max(0.0f, content.height - viewport.height)};
    }

    void ScrollState::clampOffset() noexcept
    {
        const ScrollOffset maximum = maxOffset();
        offset.x = std::clamp(offset.x, 0.0f, maximum.x);
        offset.y = std::clamp(offset.y, 0.0f, maximum.y);
    }

    bool ScrollManager::registerScrollNode(Node &node)
    {
        return states_.try_emplace(node.id()).second;
    }

    bool ScrollManager::unregisterScrollNode(
        NodeTree &nodeTree,
        Node::Id nodeId)
    {
        if (!nodeTree.findNode(nodeId))
        {
            states_.erase(nodeId);
            return false;
        }

        return states_.erase(nodeId) != 0;
    }

    bool ScrollManager::isRegistered(Node::Id nodeId) const noexcept
    {
        return states_.find(nodeId) != states_.end();
    }

    bool ScrollManager::setViewportSize(
        Node::Id nodeId,
        const LayoutSize &viewport)
    {
        auto it = states_.find(nodeId);
        if (it == states_.end())
            return false;

        it->second.viewport = {
            std::max(0.0f, viewport.width),
            std::max(0.0f, viewport.height)};
        it->second.clampOffset();
        return true;
    }

    bool ScrollManager::setContentSize(
        Node::Id nodeId,
        const LayoutSize &content)
    {
        auto it = states_.find(nodeId);
        if (it == states_.end())
            return false;

        it->second.content = {
            std::max(0.0f, content.width),
            std::max(0.0f, content.height)};
        it->second.clampOffset();
        return true;
    }

    bool ScrollManager::setOffset(
        Node::Id nodeId,
        const ScrollOffset &offset)
    {
        auto it = states_.find(nodeId);
        if (it == states_.end())
            return false;

        it->second.offset = offset;
        it->second.clampOffset();
        return true;
    }

    bool ScrollManager::scrollBy(
        Node::Id nodeId,
        const ScrollOffset &delta)
    {
        auto it = states_.find(nodeId);
        if (it == states_.end())
            return false;

        it->second.offset.x += delta.x;
        it->second.offset.y += delta.y;
        it->second.clampOffset();
        return true;
    }

    std::optional<ScrollState> ScrollManager::getState(Node::Id nodeId) const
    {
        const auto it = states_.find(nodeId);
        if (it == states_.end())
            return std::nullopt;
        return it->second;
    }

    ScrollOffset ScrollManager::getOffset(Node::Id nodeId) const noexcept
    {
        const auto it = states_.find(nodeId);
        return it == states_.end() ? ScrollOffset{} : it->second.offset;
    }

    ScrollOffset ScrollManager::getMaxOffset(Node::Id nodeId) const noexcept
    {
        const auto it = states_.find(nodeId);
        return it == states_.end() ? ScrollOffset{} : it->second.maxOffset();
    }

    ScrollOffset ScrollManager::getAccumulatedOffset(const Node &node) const noexcept
    {
        ScrollOffset result{};
        const Node *current = &node;

        while (current)
        {
            const auto it = states_.find(current->id());
            if (it != states_.end())
                result = result + it->second.offset;

            current = current->parent();
        }

        return result;
    }

    Node *ScrollManager::findNearestScrollableAncestor(
        NodeTree &nodeTree,
        Node *target) const noexcept
    {
        Node *current = target;
        while (current)
        {
            if (nodeTree.findNode(current->id()) != current)
                return nullptr;

            if (isRegistered(current->id()))
                return current;

            current = current->parent();
        }
        return nullptr;
    }

    bool ScrollManager::handleWheel(
        NodeTree &nodeTree,
        float x,
        float y,
        float deltaX,
        float deltaY,
        const Node *modalRoot)
    {
        Node *target = nodeTree.hitTest(x, y, modalRoot);
        Node *scrollNode = findNearestScrollableAncestor(nodeTree, target);
        if (!scrollNode)
            return false;

        auto it = states_.find(scrollNode->id());
        if (it == states_.end())
            return false;

        const ScrollOffset before = it->second.offset;
        it->second.offset.x += deltaX;
        it->second.offset.y += deltaY;
        it->second.clampOffset();

        return it->second.offset != before;
    }

    void ScrollManager::sync(NodeTree &nodeTree)
    {
        for (auto it = states_.begin(); it != states_.end();)
        {
            if (!nodeTree.findNode(it->first))
            {
                it = states_.erase(it);
                continue;
            }

            it->second.clampOffset();
            ++it;
        }
    }

    void ScrollManager::clear(NodeTree &nodeTree) noexcept
    {
        (void)nodeTree;
        states_.clear();
    }
}
