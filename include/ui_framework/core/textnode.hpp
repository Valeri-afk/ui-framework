#pragma once

#include "node.hpp"

#include <SDL3_ttf/SDL_ttf.h>

#include <string>

namespace ui
{

    class TextNode : public Node
    {
    public:
        TextNode() = default;
        ~TextNode() override;

        const std::string &getText() const noexcept;
        void setText(std::string text);

        TTF_Font *getFont() const noexcept;
        void setFont(TTF_Font *font);

        TextAlignment getHorizontalAlignment() const noexcept;
        void setHorizontalAlignment(TextAlignment alignment);

        TextAlignment getVerticalAlignment() const noexcept;
        void setVerticalAlignment(TextAlignment alignment);

    protected:
        LayoutSize measure(MeasureContext &ctx) override;
        void draw(SDL_Renderer *renderer) override;

    private:
        void releaseTextObject() noexcept;
        void ensureTextObject(SDL_Renderer *renderer);

        std::string text_;
        TTF_Font *font_ = nullptr;

        TextAlignment horizontalAlignment_ = TextAlignment::START;
        TextAlignment verticalAlignment_ = TextAlignment::START;

        SDL_Renderer *cachedRenderer_ = nullptr;
        TTF_TextEngine *textEngine_ = nullptr;
        TTF_Text *textObject_ = nullptr;

        friend class LayoutManager;
    };

}
