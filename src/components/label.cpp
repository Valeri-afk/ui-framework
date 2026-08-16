#include "ui_framework/components/label.hpp"
#include "ui_framework/core/node.hpp"
#include <algorithm>
#include <cmath>

namespace ui
{
    Label::Label() = default;

    Label::~Label()
    {
        if (ttfText_)
        {
            TTF_DestroyText(ttfText_);
            ttfText_ = nullptr;
        }
        if (textEngine_)
        {
            TTF_DestroyRendererTextEngine(textEngine_);
            textEngine_ = nullptr;
        }
    }

    void Label::ensureTextObject(SDL_Renderer *renderer)
    {
        if (!renderer || !font_ || text_.empty())
            return;

        if (!textEngine_ || cachedRenderer_ != renderer)
        {
            if (textEngine_)
            {
                if (ttfText_)
                {
                    TTF_DestroyText(ttfText_);
                    ttfText_ = nullptr;
                }
                TTF_DestroyRendererTextEngine(textEngine_);
                textEngine_ = nullptr;
            }

            textEngine_ = TTF_CreateRendererTextEngine(renderer);
            cachedRenderer_ = renderer;
        }

        if (textEngine_ && !ttfText_)
        {
            ttfText_ = TTF_CreateText(textEngine_, font_, text_.c_str(), text_.size());
            if (ttfText_)
            {
                SDL_Color sdlColor{color_.r, color_.g, color_.b, color_.a};
                TTF_SetTextColor(ttfText_, sdlColor.r, sdlColor.g, sdlColor.b, sdlColor.a);
                if (wrapWidth_ > 0.0f)
                {
                    TTF_SetTextWrapWidth(ttfText_, static_cast<int>(std::round(wrapWidth_)));
                }
            }
        }
    }

    void Label::setText(const std::string &text, SDL_Renderer *renderer)
    {
        if (text_ == text)
            return;
        text_ = text;

        if (ttfText_)
        {
            TTF_SetTextString(ttfText_, text_.c_str(), text_.size());
        }
        else if (renderer)
        {
            ensureTextObject(renderer);
        }
    }

    void Label::setFont(TTF_Font *font, SDL_Renderer *renderer)
    {
        if (font_ == font)
            return;
        font_ = font;

        if (ttfText_)
        {
            TTF_DestroyText(ttfText_);
            ttfText_ = nullptr;
        }
        if (renderer)
        {
            ensureTextObject(renderer);
        }
    }

    void Label::setColor(const Color &color) noexcept
    {
        color_ = color;
        if (ttfText_)
        {
            SDL_Color sdlColor{color_.r, color_.g, color_.b, color_.a};
            TTF_SetTextColor(ttfText_, sdlColor.r, sdlColor.g, sdlColor.b, sdlColor.a);
        }
    }

    void Label::setWrapWidth(float wrapWidth)
    {
        wrapWidth_ = wrapWidth;
        if (ttfText_)
        {
            TTF_SetTextWrapWidth(ttfText_, wrapWidth_ > 0.0f ? static_cast<int>(std::round(wrapWidth_)) : 0);
        }
    }

    LayoutSize Label::measure(const MeasureContext &ctx) const
    {
        if (!font_ || text_.empty())
            return {};

        // Получаем максимальную доступную ширину для текста (content-box)
        float availableWidth = ctx.constraints.maxWidth;
        if (wrapWidth_ > 0.0f)
            availableWidth = std::min(availableWidth, wrapWidth_);

        // Если есть preferredContentSize – используем его как фиксированный размер
        if (ctx.preferredContentSize)
        {
            // Возвращаем фиксированный размер, но текст всё равно может обрезаться при отрисовке
            return *ctx.preferredContentSize;
        }

        int w = 0, h = 0;
        if (availableWidth > 0.0f && availableWidth < std::numeric_limits<float>::max())
        {
            TTF_GetStringSizeWrapped(font_, text_.c_str(), text_.size(),
                                     static_cast<int>(std::round(availableWidth)), &w, &h);
        }
        else
        {
            TTF_GetStringSize(font_, text_.c_str(), text_.size(), &w, &h);
        }

        return {static_cast<float>(w), static_cast<float>(h)};
    }

    void Label::arrange(const ArrangeContext &ctx) {};

    void Label::draw(const Node &node, SDL_Renderer *renderer)
    {
        if (!renderer || text_.empty() || !font_)
            return;

        ensureTextObject(renderer);
        if (!ttfText_)
            return;

        const auto padding = node.getPadding();
        const auto pos = node.getActualPosition();
        const auto size = node.getActualSize();

        float x = pos.x + padding.left;
        float y = pos.y + padding.top;

        int w = 0, h = 0;
        TTF_GetTextSize(ttfText_, &w, &h);
        float textWidth = static_cast<float>(w);
        float textHeight = static_cast<float>(h);

        float contentWidth = std::max(0.0f, size.width - padding.left - padding.right);
        float contentHeight = std::max(0.0f, size.height - padding.top - padding.bottom);

        switch (horizontalAlignment_)
        {
        case TextAlignment::CENTER:
            x += (contentWidth - textWidth) * 0.5f;
            break;
        case TextAlignment::END:
            x += contentWidth - textWidth;
            break;
        default:
            break;
        }

        switch (verticalAlignment_)
        {
        case TextAlignment::CENTER:
            y += (contentHeight - textHeight) * 0.5f;
            break;
        case TextAlignment::END:
            y += contentHeight - textHeight;
            break;
        default:
            break;
        }

        TTF_DrawRendererText(ttfText_, x, y);
    }
}