#pragma once

#include "ui_framework/types.hpp"
#include "ui_framework/events.hpp"

#include <SDL3/SDL.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace ui
{

    class NodeTree;
    class PanelNode;
    class LayoutManager;
    class EventDispatcher;

    class Node
    {
    public:
        using Id = uint64_t;

        Node();
        virtual ~Node();

        Node(const Node &) = delete;
        Node &operator=(const Node &) = delete;

        Id id() const noexcept;

        Node *getParent() const noexcept;
        NodeTree *getNodeTree() const noexcept;

        bool isVisible() const noexcept;
        void setVisible(bool visible);

        bool isEnabled() const noexcept;
        void setEnabled(bool enabled);

        bool isFocusable() const noexcept;
        void setFocusable(bool focusable) noexcept;

        bool isCapturable() const noexcept;
        void setCapturable(bool capturable) noexcept;

        LayoutSizeValue getWidth() const noexcept;
        void setWidth(LayoutSizeValue value);

        LayoutSizeValue getHeight() const noexcept;
        void setHeight(LayoutSizeValue value);

        LayoutPosition getPosition() const noexcept;
        void setPosition(LayoutPosition position);

        PositionMode getPositionMode() const noexcept;
        void setPositionMode(PositionMode mode);

        LayoutPosition getActualPosition() const noexcept;
        LayoutSize getActualSize() const noexcept;
        LayoutSize getDesiredSize() const noexcept;

        LayoutSize getMinSize() const noexcept;
        void setMinSize(LayoutSize size);

        LayoutSize getMaxSize() const noexcept;
        void setMaxSize(LayoutSize size);

        Padding getPadding() const noexcept;
        void setPadding(Padding padding);

        Border getBorder() const noexcept;
        void setBorder(Border border);

        Overflow getOverflow() const noexcept;
        void setOverflow(Overflow overflow);

        template <typename Event>
        HandlerToken addHandler(std::function<void(Event &)> handler)
        {
            return eventHandlers_.add<Event>(std::move(handler));
        }

        template <typename Event>
        void removeHandler(HandlerToken token)
        {
            eventHandlers_.remove<Event>(token);
        }

        template <typename Event>
        void clearHandlers()
        {
            eventHandlers_.clear<Event>();
        }

    protected:
        virtual void update(float dt) {};
        virtual void draw(SDL_Renderer *renderer) {};

        virtual LayoutSize measure(MeasureContext &ctx) { return {}; }
        virtual void arrange(ArrangeContext &ctx) {}

        virtual void onMount() {};
        virtual void onUnmount() {};

        virtual Node *hitTest(float x, float y) noexcept;

        void deferLayoutMutation(std::function<void(Node &)> fn);

    private:
        Node *parent_ = nullptr;
        NodeTree *owner_ = nullptr;

        LayoutSizeValue size_{};
        LayoutPosition position_{};

        PositionMode positionMode_ = PositionMode::Layout;

        LayoutPosition actualPosition_;
        LayoutSize actualSize_;
        LayoutSize desiredSize_;

        LayoutSize minSize_{};
        LayoutSize maxSize_{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()};

        Padding padding_;
        Border border_;
        Overflow overflow_ = Overflow::VISIBLE;

        bool visible_ = true;
        bool enabled_ = true;

        bool focusable_ = false;
        bool capturable_ = false;

        EventHandlerStorage eventHandlers_;

        const Id id_ = nextId();

        static Id nextId() noexcept
        {
            static std::atomic<Id> next{1};
            return next.fetch_add(1, std::memory_order_relaxed);
        }

        LayoutSize clampSize(
            LayoutSize size,
            LayoutSize minSize,
            LayoutSize maxSize) const;

        template <typename Event>
        void dispatchEvent(Event &event, NodeTree &nodeTree);

        friend class NodeTree;
        friend class PanelNode;
        friend class LayoutManager;
        friend class EventDispatcher;
    };

}
