#include "ui_framework/components/menu.hpp"

#include <algorithm>
#include <utility>

namespace ui
{
    Menu::Menu()
    {
        setFocusable(false);
        setCapturable(false);
    }

    Node *Menu::addItem(std::unique_ptr<MenuItem> item, size_t index)
    {
        if (!item)
            return nullptr;

        MenuItem *rawItem = item.get();
        rawItem->setOnActivate(
            [this](MenuItem &activatedItem)
            {
                handleItemActivation(activatedItem);
            });

        return add(std::move(item), index);
    }

    void Menu::removeItem(MenuItem &item)
    {
        if (&item == activeItem_)
            activeItem_ = nullptr;
        if (&item == selectedItem_)
            selectedItem_ = nullptr;
        remove(item);
    }

    void Menu::setActiveItem(MenuItem *item) noexcept
    {
        if (item == activeItem_)
            return;
        syncActiveItem(item);
    }

    MenuItem *Menu::getActiveItem() const noexcept { return activeItem_; }

    void Menu::setSelectedItem(MenuItem *item) noexcept
    {
        if (item == selectedItem_)
            return;
        syncSelectedItem(item);
    }

    MenuItem *Menu::getSelectedItem() const noexcept { return selectedItem_; }

    void Menu::setItemSpacing(float spacing) noexcept
    {
        itemSpacing_ = std::max(0.0f, spacing);
        invalidateLayout();
    }

    float Menu::getItemSpacing() const noexcept { return itemSpacing_; }

    void Menu::setOnItemActivate(ItemActivationCallback callback)
    {
        onItemActivate_ = std::move(callback);
    }

    LayoutSize Menu::measureContent(const LayoutSize &availableContent) const
    {
        LayoutSize result{};
        size_t visibleCount = 0;

        for (size_t i = 0; i < getChildCount(); ++i)
        {
            Node *child = getChild(i);
            if (!child || !child->isVisible())
                continue;

            const LayoutSize childSize = child->getDesiredSize();
            result.width = std::max(result.width, childSize.width);
            result.height += childSize.height;
            ++visibleCount;
        }

        if (visibleCount > 1)
            result.height += itemSpacing_ * static_cast<float>(visibleCount - 1);

        if (availableContent.width >= 0.0f)
            result.width = std::min(result.width, availableContent.width);

        return result;
    }

    void Menu::arrangeContent(
        const LayoutPosition &contentPosition,
        const LayoutSize &contentSize)
    {
        float y = contentPosition.y;

        for (size_t i = 0; i < getChildCount(); ++i)
        {
            Node *child = getChild(i);
            if (!child || !child->isVisible())
                continue;

            const LayoutSize desired = child->getDesiredSize();
            child->setPosition({contentPosition.x, y});
            child->setSize({contentSize.width, desired.height});
            y += desired.height + itemSpacing_;
        }
    }

    void Menu::update(float)
    {
        if (activeItem_ && !activeItem_->isVisible())
            syncActiveItem(nullptr);

        if (selectedItem_ && !selectedItem_->isVisible())
            syncSelectedItem(nullptr);
    }

    void Menu::syncActiveItem(MenuItem *item) noexcept
    {
        if (activeItem_)
            activeItem_->setHighlighted(false);

        activeItem_ = item;

        if (activeItem_ && activeItem_->isEnabled())
            activeItem_->setHighlighted(true);
    }

    void Menu::syncSelectedItem(MenuItem *item) noexcept
    {
        if (selectedItem_)
            selectedItem_->setSelected(false);

        selectedItem_ = item;

        if (selectedItem_ && selectedItem_->isEnabled())
            selectedItem_->setSelected(true);
    }

    void Menu::handleItemActivation(MenuItem &item)
    {
        if (!item.isEnabled())
            return;

        setSelectedItem(&item);
        if (onItemActivate_)
            onItemActivate_(item);
    }
}
