#pragma once

#include <functional>
#include "ui_framework/types.hpp"
#include "ui_framework/components/base/widget.hpp"

namespace ui
{
    class Modal : public Widget
    {
    public:
        using OnCloseCallback = std::function<void(Modal &)>;

        Modal();
        Modal(const LayoutPosition &position, const LayoutSize &size);

        void setOnClose(OnCloseCallback callback);

        void hideBackdrop(bool hide);

        void disableTransitionEffect(bool disable);

        void onUpdate(float dt) override;
        void onDraw(SDL_Renderer *renderer) override;
        void onMouseClick(MouseClickEvent &e) override;
        void onKeyDown(KeyDownEvent &e) override;

    private:
        OnCloseCallback onCloseCb;

        bool isBackdropHide_ = false;
        bool scrollockBehavior_ = true;
        bool transitionEffect_ = true;

        const int maxBackdropAlpha = 128;
        const int maxAlpha = 255;

        Uint8 transitionAlpha = 128;

        float transitionProgress = 0.0f;
        const float transitionTime = 1.0f;

        bool wasVisible = false;
    };
}
