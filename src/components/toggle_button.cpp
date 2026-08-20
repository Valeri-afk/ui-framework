#include "ui_framework/components/toggle_button.hpp"

#include "ui_framework/core/primitives.hpp"

namespace ui
{
    ToggleButton::ToggleButton() = default;

    void ToggleButton::setSelected(bool selected) noexcept
    {
        selected_ = selected;
    }

    bool ToggleButton::isSelected() const noexcept
    {
        return selected_;
    }

    void ToggleButton::setOnToggle(std::function<void(ToggleButton &, bool)> callback)
    {
        onToggle_ = std::move(callback);
    }

    void ToggleButton::toggle()
    {
        if (!isEnabled() || !isVisible())
            return;

        setSelected(!selected_);

        if (onToggle_)
            onToggle_(*this, selected_);
    }

    void ToggleButton::activate()
    {
        if (!isVisible() || !isEnabled())
            return;

        toggle();
        Button::onActivate();
    }

    void ToggleButton::draw(SDL_Renderer *renderer)
    {
        if (!renderer)
            return;

        Button::draw(renderer);
    }
}
