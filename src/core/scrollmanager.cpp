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

    std::optional<ScrollState> ScrollManager::getState(
        Node::Id nodeId) const
    {
        const auto it = states_.find(nodeId);
        if (it == states_.end())
            return std::nullopt;

        return it->second;
    }

    ScrollOffset ScrollManager::getOffset(Node::Id nodeId) const noexcept
    {
        const auto it = states_.find(nodeId);
        if (it == states_.end())
            return {};

        return it->second.offset;
    }

    ScrollOffset ScrollManager::getMaxOffset(Node::Id nodeId) const noexcept
    {
        const auto it = states_.find(nodeId);
        if (it == states_.end())
            return {};

        return it->second.maxOffset();
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
