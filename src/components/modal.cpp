#include <iostream>
#include "ui_framework/components/base/primitives.hpp"
#include "ui_framework/components/modal.hpp"

namespace ui
{
    Modal::Modal() : Widget()
    {
        setPreferredSize({600, 400});
        setBorderRadius(4.0f);
        setPadding({10, 10, 10, 10});
        setLayoutType(LayoutType::HORIZONTAL);
        setOverflow(Overflow::VISIBLE);
        setBackgroundColor(Colors::white);
        wasVisible = isVisible();
    };

    Modal::Modal(const LayoutPosition &position, const LayoutSize &size) : Modal()
    {
        setPreferredPosition(position);
        setPreferredSize(size);
    };

    void Modal::setOnClose(OnCloseCallback callback) { onCloseCb = callback; }
    void Modal::hideBackdrop(bool hide) { isBackdropHide_ = hide; }

    void Modal::disableTransitionEffect(bool disable)
    {
        transitionEffect_ = !disable;
        auto [r, g, b, a] = getBackgroundColor();
        setBackgroundColor({r, g, b, 255});
    }

    void Modal::onMouseClick(MouseClickEvent &e)
    {
        if (!onCloseCb)
            return;

        if (e.button == MouseButton::LEFT)
            onCloseCb(*this);
    };

    void Modal::onKeyDown(KeyDownEvent &e)
    {
        if (!onCloseCb)
            return;

        if (e.key == KeyCode::ESCAPE)
            onCloseCb(*this);
    };

    void Modal::onUpdate(float deltaTime)
    {
        if (wasVisible && !visible_)
        {
            transitionProgress = 0.0f;
            transitionAlpha = 128;
        }

        wasVisible = visible_;

        if (transitionEffect_)
        {
            if (transitionProgress < 1.0f)
            {
                float speed = (maxBackdropAlpha * deltaTime) / transitionTime;
                transitionProgress += speed / maxBackdropAlpha;

                transitionAlpha = maxBackdropAlpha * transitionProgress;
                auto [r, g, b, a] = getBackgroundColor();

                a = std::min(maxBackdropAlpha + transitionAlpha, maxAlpha);

                setBackgroundColor({r, g, b, a});

                if (transitionProgress > 1.0f)
                    transitionProgress = 1.0f;
            }
        }
    }

    void Modal::onDraw(SDL_Renderer *renderer)
    {
        if (!renderer)
            return;

        if (!isBackdropHide_)
        {
            int screenW, screenH;
            SDL_GetCurrentRenderOutputSize(renderer, &screenW, &screenH);
            primitives::boxRGBA(renderer, 0.0f, 0.0f, screenW, screenH, 0, 0, 0, transitionAlpha);
        }
    };
}
