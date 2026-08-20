#pragma once

#include <cstddef>
#include <functional>
#include <optional>

#include "ui_framework/core/panelnode.hpp"
#include "ui_framework/components/menu_item.hpp"

namespace ui
{
    class Menu : public PanelNode
    {
    public:
        using ItemActivationCallback = std::function<void(MenuItem &)>;

        Menu();
        ~Menu() override = default;

        Node *addItem(std::unique_ptr<MenuItem> item, size_t index = static_cast<size_t>(-1));
        void removeItem(MenuItem &item);

        void setActiveItem(MenuItem *item) noexcept;
        MenuItem *getActiveItem() const noexcept;

        void setSelectedItem(MenuItem *item) noexcept;
        MenuItem *getSelectedItem() const noexcept;

        void setItemSpacing(float spacing) noexcept;
        float getItemSpacing() const noexcept;

        void setOnItemActivate(ItemActivationCallback callback);

    protected:
        LayoutSize measureContent(const LayoutSize &availableContent) const override;
        void arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize) override;
        void update(float dt) override;

    private:
        void syncActiveItem(MenuItem *item) noexcept;
        void syncSelectedItem(MenuItem *item) noexcept;
        void handleItemEnter(MenuItem &item);
        void handleItemLeave(MenuItem &item);
        void handleItemActivation(MenuItem &item);

        MenuItem *activeItem_ = nullptr;
        MenuItem *selectedItem_ = nullptr;
        float itemSpacing_ = 0.0f;
        ItemActivationCallback onItemActivate_;
    };
}
