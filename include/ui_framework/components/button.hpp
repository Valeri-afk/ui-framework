#pragma once

#include <string>
#include <functional>
#include <memory>
#include <SDL3_ttf/SDL_ttf.h>
#include "ui_framework/components/component.hpp"
#include "ui_framework/components/label.hpp"
#include "ui_framework/types.hpp"

namespace ui
{
    class Button : public Component
    {
    public:
        enum class Type
        {
            FILLED,   // залитый фон
            OUTLINED, // только рамка
            TEXT      // только текст, без фона и рамки
        };

        using OnClickCallback = std::function<void(MouseClickEvent &)>;
        using OnMouseDownCallback = std::function<void(MouseDownEvent &)>;
        using OnMouseUpCallback = std::function<void(MouseUpEvent &)>;
        using OnMouseEnterCallback = std::function<void(MouseEnterEvent &)>;
        using OnMouseLeaveCallback = std::function<void(MouseLeaveEvent &)>;

        Button() = default;
        explicit Button(float borderRadius);

        // Прокси к Label
        void setText(const std::string &text, SDL_Renderer *renderer = nullptr);
        const std::string &getText() const;

        void setFont(TTF_Font *font, SDL_Renderer *renderer = nullptr);
        TTF_Font *getFont() const;

        void setTextColor(const Color &color);
        Color getTextColor() const;

        void setTextAlignment(TextAlignment hAlign, TextAlignment vAlign);
        TextAlignment getHorizontalTextAlignment() const;
        TextAlignment getVerticalTextAlignment() const;

        // Стили кнопки
        void setBackgroundColor(const Color &color);
        Color getBackgroundColor() const;

        void setBorderColor(const Color &color);
        Color getBorderColor() const;

        void setBorderRadius(float radius);
        float getBorderRadius() const;

        // Тип и масштаб
        void setType(Type type);
        Type getType() const { return type_; }

        void setResizeScale(float scale);
        void setResizeEffectEnabled(bool enabled);
        float getScale() const { return currentScale_; } // текущий масштаб (для отладки)

        // Колбэки
        void setOnClick(OnClickCallback cb);
        void setOnMouseDown(OnMouseDownCallback cb);
        void setOnMouseUp(OnMouseUpCallback cb);
        void setOnMouseEnter(OnMouseEnterCallback cb);
        void setOnMouseLeave(OnMouseLeaveCallback cb);

        // Component overrides
        void onMount(Node &node) override;
        void onUnmount(Node &node) override;
        void update(Node &node, float dt) override;
        LayoutSize measure(const MeasureContext &ctx) const override;
        void arrange(const ArrangeContext &ctx) override;
        void draw(const Node &node, SDL_Renderer *renderer) override;

        // События
        void onMouseDown(Node &node, MouseDownEvent &event) override;
        void onMouseUp(Node &node, MouseUpEvent &event) override;
        void onMouseClick(Node &node, MouseClickEvent &event) override;
        void onMouseEnter(Node &node, MouseEnterEvent &event) override;
        void onMouseLeave(Node &node, MouseLeaveEvent &event) override;

    private:
        void createLabel(Node &node);

        // Дочерний Label
        Node *labelNode_ = nullptr;
        Label *label_ = nullptr;

        // Стили
        Color backgroundColor_ = Colors::gray;
        Color borderColor_ = Colors::black;
        float borderRadius_ = 4.0f;
        Type type_ = Type::FILLED;

        // Масштабирование (визуальный эффект)
        float resizeScale_ = 1.1f;
        bool resizeEnabled_ = true;
        mutable float currentScale_ = 1.0f;
        mutable float targetScale_ = 1.0f;
        mutable bool pressed_ = false;

        // Колбэки
        OnClickCallback onClickCb_;
        OnMouseDownCallback onMouseDownCb_;
        OnMouseUpCallback onMouseUpCb_;
        OnMouseEnterCallback onMouseEnterCb_;
        OnMouseLeaveCallback onMouseLeaveCb_;
    };
}