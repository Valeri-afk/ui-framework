#pragma once

#include <SDL3/SDL.h>

#include "ui_framework/core/node.hpp"
#include "ui_framework/types.hpp"

namespace ui
{

    class ControlNode : public Node
    {
    public:
        ControlNode() = default;
        ~ControlNode() override = default;

        void setStyleProps(const StyleProps &style) noexcept;
        StyleProps getStyleProps() const noexcept;

    protected:
        void drawSelf(SDL_Renderer *renderer) override;

        StyleProps styleProps_;
    };

}