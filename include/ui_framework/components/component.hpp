#pragma once

#include <SDL3/SDL.h>

#include "ui_framework/event_types.hpp"
#include "ui_framework/types.hpp"

namespace ui
{
    class Node;

    class Component
    {
    public:
        virtual ~Component() = default;

        virtual void onMount(Node &node) {}
        virtual void onUnmount(Node &node) {}

        virtual void update(Node &node, float dt) {}

        virtual void draw(const Node &node, SDL_Renderer *renderer) {}

        /*
            Component возвращает content-box intrinsic size.

            Framework сам:
                - добавит padding/border;
                - применит requested/preferred size;
                - применит min/max;
                - применит parent constraints.
        */
        virtual LayoutSize measure(const MeasureContext &ctx) const
        {
            return {};
        }

        /*
            Component раскладывает только своих непосредственных детей.

            Component не получает Node.
            Component не меняет actualPosition/actualSize напрямую.
            Component использует ctx.placeChild(...).
        */
        virtual void arrange(const ArrangeContext &ctx) {}

        virtual void onMouseDown(Node &, MouseDownEvent &) {}
        virtual void onMouseUp(Node &, MouseUpEvent &) {}
        virtual void onMouseClick(Node &, MouseClickEvent &) {}
        virtual void onMouseMove(Node &, MouseMoveEvent &) {}
        virtual void onMouseEnter(Node &, MouseEnterEvent &) {}
        virtual void onMouseLeave(Node &, MouseLeaveEvent &) {}
        virtual void onMouseWheel(Node &, MouseWheelEvent &) {}

        virtual void onKeyDown(Node &, KeyDownEvent &) {}
        virtual void onKeyUp(Node &, KeyUpEvent &) {}

        virtual void onFocusGained(Node &, FocusGainedEvent &) {}
        virtual void onFocusLost(Node &, FocusLostEvent &) {}

        virtual void onDragBegin(Node &, MouseDragBeginEvent &) {}
        virtual void onDrag(Node &, MouseDragEvent &) {}
        virtual void onDragEnd(Node &, MouseDragEndEvent &) {}
    };
}