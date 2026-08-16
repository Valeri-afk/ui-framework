#include "ui_framework/components/menu.hpp"
#include "ui_framework/components/base/primitives.hpp"
#include <algorithm>

#include <iostream>

namespace ui
{
    Menu::Menu() : Widget()
    {
        setPreferredSize({200, 60});
        setBackgroundColor(Colors::white);
        setLayoutType(LayoutType::VERTICAL);
        setTopPadding(20);
        setBottomPadding(20);
        setSizeMode(SizeMode::CONTENT);
        activeItemColor_ = Colors::gray;
    }

    Menu::Menu(const LayoutPosition &position, const LayoutSize &size) : Menu()
    {
        setPreferredPosition(position);
        setPreferredSize(size);
    }

    void Menu::setActiveItem(size_t ind)
    {
        if (ind < childCount())
            activeItem_ = ind;
    };

    void Menu::onMouseDown(MouseDownEvent &e)
    {
        if (onMouseDownCb_)
            onMouseDownCb_(e);
    };
    void Menu::onMouseUp(MouseUpEvent &e)
    {
        if (onMouseUpCb_)
            onMouseUpCb_(e);
    };
    void Menu::onMouseClick(MouseClickEvent &e)
    {
        if (onClickCb_)
            onClickCb_(e);
    };
    void Menu::onMouseLeave(MouseLeaveEvent &e)
    {
        if (onMouseLeaveCb_)
            onMouseLeaveCb_(e);
    };

    void Menu::onMouseEnter(MouseEnterEvent &e)
    {
        if (onMouseEnterCb_)
            onMouseEnterCb_(e);

        if (activeItem_.has_value())
        {
            size_t i = activeItem_.value();

            Widget *child = getChild(i);
            child->setBackgroundColor(activeItemColor_);
        }
    }

}
