#pragma once

#include <string>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "ui_framework/components/component.hpp"
#include "ui_framework/types.hpp"

namespace ui
{
    class Label : public Component
    {
    public:
        Label();
        ~Label() override;

        void setText(const std::string &text, SDL_Renderer *renderer = nullptr);
        const std::string &getText() const noexcept { return text_; }

        void setFont(TTF_Font *font, SDL_Renderer *renderer = nullptr);
        TTF_Font *getFont() const noexcept { return font_; }

        void setColor(const Color &color) noexcept;
        const Color &getColor() const noexcept { return color_; }

        void setHorizontalAlignment(TextAlignment alignment) noexcept { horizontalAlignment_ = alignment; }
        TextAlignment getHorizontalAlignment() const noexcept { return horizontalAlignment_; }

        void setVerticalAlignment(TextAlignment alignment) noexcept { verticalAlignment_ = alignment; }
        TextAlignment getVerticalAlignment() const noexcept { return verticalAlignment_; }

        void setWrapWidth(float wrapWidth);
        float getWrapWidth() const noexcept { return wrapWidth_; }

        LayoutSize measure(const MeasureContext &ctx) const override;
        void arrange(const ArrangeContext &ctx) override;
        void draw(const Node &node, SDL_Renderer *renderer) override;

    private:
        void ensureTextObject(SDL_Renderer *renderer);

        std::string text_;
        TTF_Font *font_ = nullptr;
        Color color_ = Colors::white;
        TextAlignment horizontalAlignment_ = TextAlignment::START;
        TextAlignment verticalAlignment_ = TextAlignment::START;
        float wrapWidth_ = 0.0f;

        mutable TTF_TextEngine *textEngine_ = nullptr;
        mutable TTF_Text *ttfText_ = nullptr;
        mutable SDL_Renderer *cachedRenderer_ = nullptr;
    };
}