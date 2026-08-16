#include "ui_framework/core/controlnode.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{

    float finiteOrZero(float value) noexcept
    {
        return std::isfinite(value) ? value : 0.0f;
    }

    int clampToInt(float value) noexcept
    {
        if (!std::isfinite(value))
            return 0;

        if (value <= static_cast<float>(std::numeric_limits<int>::min()))
            return std::numeric_limits<int>::min();

        if (value >= static_cast<float>(std::numeric_limits<int>::max()))
            return std::numeric_limits<int>::max();

        return static_cast<int>(value);
    }

    ui::SDL_Rect toSDLRect(const ui::Node &node)
    {
        const ui::LayoutPosition position = node.getActualPosition();
        const ui::LayoutSize size = node.getActualSize();

        const float safeX = finiteOrZero(position.x);
        const float safeY = finiteOrZero(position.y);
        const float safeWidth = std::max(0.0f, finiteOrZero(size.width));
        const float safeHeight = std::max(0.0f, finiteOrZero(size.height));

        const int x = clampToInt(std::floor(safeX));
        const int y = clampToInt(std::floor(safeY));

        const int right =
            clampToInt(std::ceil(safeX + safeWidth));

        const int bottom =
            clampToInt(std::ceil(safeY + safeHeight));

        return {
            x,
            y,
            std::max(0, right - x),
            std::max(0, bottom - y)};
    }

    void setDrawColor(
        SDL_Renderer *renderer,
        const ui::Color &color)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(
            renderer,
            color.r,
            color.g,
            color.b,
            color.a);
    }

    class RendererDrawStateScope
    {
    public:
        explicit RendererDrawStateScope(SDL_Renderer *renderer) noexcept
            : renderer_(renderer)
        {
            if (!renderer_)
                return;

            SDL_GetRenderDrawBlendMode(renderer_, &previousBlendMode_);

            SDL_GetRenderDrawColor(
                renderer_,
                &previousR_,
                &previousG_,
                &previousB_,
                &previousA_);
        }

        ~RendererDrawStateScope()
        {
            if (!renderer_)
                return;

            SDL_SetRenderDrawBlendMode(renderer_, previousBlendMode_);

            SDL_SetRenderDrawColor(
                renderer_,
                previousR_,
                previousG_,
                previousB_,
                previousA_);
        }

        RendererDrawStateScope(const RendererDrawStateScope &) = delete;
        RendererDrawStateScope &operator=(const RendererDrawStateScope &) = delete;

    private:
        SDL_Renderer *renderer_ = nullptr;

        SDL_BlendMode previousBlendMode_ = SDL_BLENDMODE_BLEND;

        Uint8 previousR_ = 255;
        Uint8 previousG_ = 255;
        Uint8 previousB_ = 255;
        Uint8 previousA_ = 255;
    };

}

namespace ui
{

    void ControlNode::setStyleProps(
        const StyleProps &style) noexcept
    {
        styleProps_ = style;
    }

    StyleProps ControlNode::getStyleProps() const noexcept
    {
        return styleProps_;
    }

    void ControlNode::drawSelf(SDL_Renderer *renderer)
    {
        if (!renderer)
            return;

        RendererDrawStateScope state(renderer);

        const SDL_Rect rect = toSDLRect(*this);

        if (rect.w <= 0 || rect.h <= 0)
            return;

        if (styleProps_.backgroundColor.a > 0)
        {
            setDrawColor(renderer, styleProps_.backgroundColor);
            SDL_RenderFillRect(renderer, &rect);
        }

        const float borderWidth =
            std::max(0.0f, finiteOrZero(styleProps_.borderWidth));

        if (borderWidth > 0.0f &&
            styleProps_.borderColor.a > 0)
        {
            const int border =
                std::max(1, static_cast<int>(std::ceil(borderWidth)));

            const SDL_Rect top{
                rect.x,
                rect.y,
                rect.w,
                std::min(border, rect.h)};

            const SDL_Rect bottom{
                rect.x,
                std::max(rect.y, rect.y + rect.h - border),
                rect.w,
                std::min(border, rect.h)};

            const SDL_Rect left{
                rect.x,
                rect.y,
                std::min(border, rect.w),
                rect.h};

            const SDL_Rect right{
                std::max(rect.x, rect.x + rect.w - border),
                rect.y,
                std::min(border, rect.w),
                rect.h};

            setDrawColor(renderer, styleProps_.borderColor);

            SDL_RenderFillRect(renderer, &top);
            SDL_RenderFillRect(renderer, &bottom);
            SDL_RenderFillRect(renderer, &left);
            SDL_RenderFillRect(renderer, &right);
        }
    }

}