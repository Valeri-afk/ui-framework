#pragma once

#include <optional>
#include <functional>
#include "ui_framework/components/base/widget.hpp"

namespace ui
{
    class Menu : public Widget
    {
    public:
        using OnClickCallback = std::function<void(MouseClickEvent &)>;
        using OnMouseDownCallback = std::function<void(MouseDownEvent &)>;
        using OnMouseUpCallback = std::function<void(MouseUpEvent &)>;
        using OnMouseLeaveCallback = std::function<void(MouseLeaveEvent &)>;
        using OnMouseEnterCallback = std::function<void(MouseEnterEvent &)>;

        Menu();
        Menu(const LayoutPosition &position, const LayoutSize &size);

        void setActiveItem(size_t ind);
        std::optional<size_t> getActiveItem() { return activeItem_; };

        void setActiveItemColor(const Color &color) { activeItemColor_ = color; }

        void setOnClick(OnClickCallback callback) { onClickCb_ = callback; };
        void setOnMouseDown(OnMouseDownCallback callback) { onMouseDownCb_ = callback; };
        void setOnMouseUp(OnMouseUpCallback callback) { onMouseUpCb_ = callback; };
        void setOnMouseLeave(OnMouseLeaveCallback callback) { onMouseLeaveCb_ = callback; };
        void setOnMouseEnter(OnMouseEnterCallback callback) { onMouseEnterCb_ = callback; };

    protected:
        void onMouseDown(MouseDownEvent &e) override;
        void onMouseUp(MouseUpEvent &e) override;
        void onMouseClick(MouseClickEvent &e) override;
        void onMouseLeave(MouseLeaveEvent &e) override;
        void onMouseEnter(MouseEnterEvent &e) override;

    private:
        std::optional<size_t> activeItem_;

        OnClickCallback onClickCb_;
        OnMouseDownCallback onMouseDownCb_;
        OnMouseUpCallback onMouseUpCb_;
        OnMouseLeaveCallback onMouseLeaveCb_;
        OnMouseEnterCallback onMouseEnterCb_;

        Color activeItemColor_;
        Color originalColor_;
    };
}
