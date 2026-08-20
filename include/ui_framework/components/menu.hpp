#pragma once

#include <cstddef>
#include <functional>

#include "ui_framework/components/menu_item.hpp"
#include "ui_framework/core/stackpanelnode.hpp"

namespace ui
{
    class Menu : public StackPanelNode
    {
    public:
        using ItemActivationCallback = std::function<void(MenuItem &)>;

        Menu();
        ~Menu() override = default;

        MenuItem *addItem(std::unique_ptr<MenuItem> item, size_t index = static_cast<size_t>(-1));
        void removeItem(MenuItem &item);

        void setActiveItem(MenuItem *item) noexcept;
        MenuItem *getActiveItem() const noexcept;

        void setSelectedItem(MenuItem *item) noexcept;
        MenuItem *getSelectedItem() const noexcept;

        void setItemSpacing(float spacing);
        float getItemSpacing() const noexcept;

        void setOnItemActivate(ItemActivationCallback callback);

    private:
        void handleMouseEnter(MouseEnterEvent &event);
        void handleMouseLeave(MouseLeaveEvent &event);
        void handleMouseClick(MouseClickEvent &event);

        MenuItem *asMenuItem(Node *node) const noexcept;

        MenuItem *activeItem_ = nullptr;
        MenuItem *selectedItem_ = nullptr;
        ItemActivationCallback onItemActivate_;
    };
}
