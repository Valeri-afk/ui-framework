#include "ui_framework/core/textnode.hpp"

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
}

namespace ui
{

    TextNode::~TextNode()
    {
        releaseTextObject();
    }

    const std::string &TextNode::getText() const noexcept
    {
        return text_;
    }

    void TextNode::setText(std::string text)
    {
        if (text_ == text)
            return;

        deferLayoutMutation(
            [text = std::move(text)](Node &node)
            {
                auto &textNode = static_cast<TextNode &>(node);
                textNode.text_ = text;

                if (textNode.textObject_)
                    TTF_SetTextString(
                        textNode.textObject_,
                        textNode.text_.c_str(),
                        textNode.text_.size());
            });
    }

    TTF_Font *TextNode::getFont() const noexcept
    {
        return font_;
    }

    void TextNode::setFont(TTF_Font *font)
    {
        if (font_ == font)
            return;

        deferLayoutMutation(
            [font](Node &node)
            {
                auto &textNode = static_cast<TextNode &>(node);
                textNode.font_ = font;
                textNode.releaseTextObject();
            });
    }

    TextAlignment TextNode::getHorizontalAlignment() const noexcept
    {
        return horizontalAlignment_;
    }

    void TextNode::setHorizontalAlignment(TextAlignment alignment)
    {
        if (horizontalAlignment_ == alignment)
            return;

        deferLayoutMutation(
            [alignment](Node &node)
            {
                static_cast<TextNode &>(node).horizontalAlignment_ = alignment;
            });
    }

    TextAlignment TextNode::getVerticalAlignment() const noexcept
    {
        return verticalAlignment_;
    }

    void TextNode::setVerticalAlignment(TextAlignment alignment)
    {
        if (verticalAlignment_ == alignment)
            return;

        deferLayoutMutation(
            [alignment](Node &node)
            {
                static_cast<TextNode &>(node).verticalAlignment_ = alignment;
            });
    }

    LayoutSize TextNode::measure(MeasureContext &ctx)
    {
        if (!font_ || text_.empty())
            return {};

        const float availableWidth =
            finiteOrInfinity(ctx.availableSize.width);

        int width = 0;
        int height = 0;

        const bool measured =
            availableWidth < kInfinity
                ? TTF_GetStringSizeWrapped(
                      font_,
                      text_.c_str(),
                      text_.size(),
                      static_cast<int>(std::round(
                          std::max(0.0f, availableWidth))),
                      &width,
                      &height)
                : TTF_GetStringSize(
                      font_,
                      text_.c_str(),
                      text_.size(),
                      &width,
                      &height);

        if (!measured)
            return {};

        return {
            static_cast<float>(width),
            static_cast<float>(height)};
    }

    void TextNode::draw(SDL_Renderer *renderer)
    {
        if (!renderer || !font_ || text_.empty())
            return;

        ensureTextObject(renderer);

        if (!textObject_)
            return;

        const Padding padding = getPadding();
        const Border border = getBorder();
        const LayoutPosition position = getActualPosition();
        const LayoutSize size = getActualSize();

        const float contentX =
            position.x + border.left + padding.left;
        const float contentY =
            position.y + border.top + padding.top;

        const float contentWidth = std::max(
            0.0f,
            size.width -
                border.left - border.right -
                padding.left - padding.right);

        const float contentHeight = std::max(
            0.0f,
            size.height -
                border.top - border.bottom -
                padding.top - padding.bottom);

        int textWidth = 0;
        int textHeight = 0;
        TTF_GetTextSize(textObject_, &textWidth, &textHeight);

        float x = contentX;
        float y = contentY;

        switch (horizontalAlignment_)
        {
        case TextAlignment::CENTER:
            x += (contentWidth - static_cast<float>(textWidth)) * 0.5f;
            break;
        case TextAlignment::END:
            x += contentWidth - static_cast<float>(textWidth);
            break;
        case TextAlignment::START:
            break;
        }

        switch (verticalAlignment_)
        {
        case TextAlignment::CENTER:
            y += (contentHeight - static_cast<float>(textHeight)) * 0.5f;
            break;
        case TextAlignment::END:
            y += contentHeight - static_cast<float>(textHeight);
            break;
        case TextAlignment::START:
            break;
        }

        TTF_DrawRendererText(textObject_, x, y);
    }

    void TextNode::releaseTextObject() noexcept
    {
        if (textObject_)
        {
            TTF_DestroyText(textObject_);
            textObject_ = nullptr;
        }

        if (textEngine_)
        {
            TTF_DestroyRendererTextEngine(textEngine_);
            textEngine_ = nullptr;
        }

        cachedRenderer_ = nullptr;
    }

    void TextNode::ensureTextObject(SDL_Renderer *renderer)
    {
        if (!renderer || !font_ || text_.empty())
            return;

        if (!textEngine_ || cachedRenderer_ != renderer)
        {
            releaseTextObject();
            textEngine_ = TTF_CreateRendererTextEngine(renderer);
            cachedRenderer_ = renderer;
        }

        if (!textEngine_ || textObject_)
            return;

        textObject_ = TTF_CreateText(
            textEngine_,
            font_,
            text_.c_str(),
            text_.size());
    }

}
