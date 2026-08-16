#include "ui_framework/components/button.hpp"
#include "ui_framework/core/node.hpp"
#include "ui_framework/core/primitives.hpp"
#include <algorithm>
#include <cmath>

namespace ui
{
    Button::Button(float borderRadius)
        : borderRadius_(std::max(0.0f, borderRadius))
    {
    }

    // ============================================================
    // Прокси к Label
    // ============================================================

    void Button::setText(const std::string &text, SDL_Renderer *renderer)
    {
        if (label_)
            label_->setText(text, renderer);
    }

    const std::string &Button::getText() const
    {
        static const std::string empty;
        return label_ ? label_->getText() : empty;
    }

    void Button::setFont(TTF_Font *font, SDL_Renderer *renderer)
    {
        if (label_)
            label_->setFont(font, renderer);
    }

    TTF_Font *Button::getFont() const
    {
        return label_ ? label_->getFont() : nullptr;
    }

    void Button::setTextColor(const Color &color)
    {
        if (label_)
            label_->setColor(color);
    }

    Color Button::getTextColor() const
    {
        return label_ ? label_->getColor() : Colors::white;
    }

    void Button::setTextAlignment(TextAlignment hAlign, TextAlignment vAlign)
    {
        if (label_)
        {
            label_->setHorizontalAlignment(hAlign);
            label_->setVerticalAlignment(vAlign);
        }
    }

    TextAlignment Button::getHorizontalTextAlignment() const
    {
        return label_ ? label_->getHorizontalAlignment() : TextAlignment::START;
    }

    TextAlignment Button::getVerticalTextAlignment() const
    {
        return label_ ? label_->getVerticalAlignment() : TextAlignment::START;
    }

    // ============================================================
    // Стили кнопки
    // ============================================================

    void Button::setBackgroundColor(const Color &color) { backgroundColor_ = color; }
    Color Button::getBackgroundColor() const { return backgroundColor_; }

    void Button::setBorderColor(const Color &color) { borderColor_ = color; }
    Color Button::getBorderColor() const { return borderColor_; }

    void Button::setBorderRadius(float radius) { borderRadius_ = std::max(0.0f, radius); }
    float Button::getBorderRadius() const { return borderRadius_; }

    void Button::setType(Type type) { type_ = type; }

    void Button::setResizeScale(float scale) { resizeScale_ = std::max(0.0f, scale); }
    void Button::setResizeEffectEnabled(bool enabled) { resizeEnabled_ = enabled; }

    // ============================================================
    // Колбэки
    // ============================================================

    void Button::setOnClick(OnClickCallback cb) { onClickCb_ = std::move(cb); }
    void Button::setOnMouseDown(OnMouseDownCallback cb) { onMouseDownCb_ = std::move(cb); }
    void Button::setOnMouseUp(OnMouseUpCallback cb) { onMouseUpCb_ = std::move(cb); }
    void Button::setOnMouseEnter(OnMouseEnterCallback cb) { onMouseEnterCb_ = std::move(cb); }
    void Button::setOnMouseLeave(OnMouseLeaveCallback cb) { onMouseLeaveCb_ = std::move(cb); }

    // ============================================================
    // Жизненный цикл
    // ============================================================

    void Button::createLabel(Node &node)
    {
        auto childNode = std::make_unique<Node>();
        auto labelComponent = std::make_unique<Label>();
        label_ = labelComponent.get();
        childNode->setComponent(std::move(labelComponent));

        labelNode_ = node.attachChild(std::move(childNode), 0);
        if (labelNode_ && label_)
        {
            // Настройки по умолчанию
            label_->setVerticalAlignment(TextAlignment::CENTER);
            label_->setHorizontalAlignment(TextAlignment::CENTER);
            label_->setColor(Colors::white);
        }
    }

    void Button::onMount(Node &node)
    {
        createLabel(node);
    }

    void Button::onUnmount(Node &node)
    {
        if (labelNode_)
        {
            node.detachChild(labelNode_);
            labelNode_ = nullptr;
            label_ = nullptr;
        }
    }

    void Button::update(Node & /*node*/, float dt)
    {
        targetScale_ = pressed_ ? resizeScale_ : 1.0f;

        if (resizeEnabled_)
        {
            const float speed = 0.15f * 60.0f * dt;
            currentScale_ += (targetScale_ - currentScale_) * speed;
            if (std::abs(currentScale_ - targetScale_) < 0.001f)
                currentScale_ = targetScale_;
        }
        else
        {
            currentScale_ = 1.0f;
        }
    }

    // ============================================================
    // Измерение и расположение
    // ============================================================

    LayoutSize Button::measure(const MeasureContext &ctx) const
    {
        if (ctx.preferredContentSize)
            return *ctx.preferredContentSize;

        if (!labelNode_)
            return {};

        // Передаём Label те же ограничения, что и кнопке (content-box)
        return ctx.measureChild(0, ctx.constraints);
    }

    void Button::arrange(const ArrangeContext &ctx)
    {
        if (!labelNode_ || ctx.childrenMeasure.empty())
            return;

        LayoutSize childSize = ctx.childrenMeasure[0];
        LayoutPosition childPos = ctx.contentPosition;

        // Центрируем текст внутри контента кнопки
        const float availableW = ctx.contentSize.width;
        const float availableH = ctx.contentSize.height;
        const float textW = childSize.width;
        const float textH = childSize.height;

        childPos.x += (availableW - textW) * 0.5f;
        childPos.y += (availableH - textH) * 0.5f;

        ctx.placeChild(0, childPos, childSize);
    }

    // ============================================================
    // Отрисовка
    // ============================================================

    void Button::draw(const Node &node, SDL_Renderer *renderer)
    {
        if (!renderer)
            return;

        const LayoutPosition pos = node.getActualPosition();
        const LayoutSize size = node.getActualSize();

        const float w = size.width * currentScale_;
        const float h = size.height * currentScale_;
        const float ox = (w - size.width) * 0.5f;
        const float oy = (h - size.height) * 0.5f;

        const float x = pos.x - ox;
        const float y = pos.y - oy;
        const float rad = borderRadius_ * currentScale_;

        // Рисуем фон (только для FILLED)
        if (type_ == Type::FILLED)
        {
            const Color bg = backgroundColor_;
            if (rad > 0.0f)
                primitives::roundedBoxRGBA(renderer, x, y, x + w, y + h, rad, bg.r, bg.g, bg.b, bg.a);
            else
                primitives::boxRGBA(renderer, x, y, x + w, y + h, bg.r, bg.g, bg.b, bg.a);
        }

        // Рисуем рамку (для OUTLINED и, опционально, для FILLED если borderColor не прозрачный)
        if (type_ == Type::OUTLINED || (type_ == Type::FILLED && borderColor_.a > 0))
        {
            const Color brd = borderColor_;
            if (rad > 0.0f)
                primitives::roundedRectangleRGBA(renderer, x, y, x + w, y + h, rad, brd.r, brd.g, brd.b, brd.a);
            else
                primitives::rectangleRGBA(renderer, x, y, x + w, y + h, brd.r, brd.g, brd.b, brd.a);
        }

        // Type::TEXT – ничего не рисуем, только Label
    }

    // ============================================================
    // Обработчики событий
    // ============================================================

    void Button::onMouseDown(Node & /*node*/, MouseDownEvent &event)
    {
        if (event.button == MouseButton::Left)
        {
            pressed_ = true;
            if (onMouseDownCb_)
                onMouseDownCb_(event);
        }
    }

    void Button::onMouseUp(Node & /*node*/, MouseUpEvent &event)
    {
        if (event.button == MouseButton::Left)
        {
            pressed_ = false;
            if (onMouseUpCb_)
                onMouseUpCb_(event);
        }
    }

    void Button::onMouseClick(Node & /*node*/, MouseClickEvent &event)
    {
        if (event.button == MouseButton::Left)
        {
            if (onClickCb_)
                onClickCb_(event);
        }
    }

    void Button::onMouseEnter(Node & /*node*/, MouseEnterEvent &event)
    {
        if (onMouseEnterCb_)
            onMouseEnterCb_(event);
    }

    void Button::onMouseLeave(Node & /*node*/, MouseLeaveEvent &event)
    {
        pressed_ = false;
        if (onMouseLeaveCb_)
            onMouseLeaveCb_(event);
    }
}