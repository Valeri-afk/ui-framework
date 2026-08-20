#pragma once

#include "ui_framework/components/button.hpp"

namespace ui
{
    class ToggleButton : public Button
    {
    public:
        ToggleButton();
        ~ToggleButton() override = default;

        void setSelected(bool selected) noexcept;
        bool isSelected() const noexcept;

        void setOnToggle(std::function<void(ToggleButton &, bool)> callback);

        void toggle();

    protected:
        void draw(SDL_Renderer *renderer) override;

    private:
        bool selected_ = false;
        std::function<void(ToggleButton &, bool)> onToggle_;
    };
}
