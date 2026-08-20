#include "modalmanager.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

#include <SDL3/SDL.h>

namespace ui
{
    class ModalManager::BackdropNode final : public Node
    {
    public:
        void setBackdrop(const Color &color, float opacity) noexcept
        {
            color_ = color;
            opacity_ = std::clamp(opacity, 0.0f, 1.0f);
        }

        void setViewport(const LayoutSize &size)
        {
            setPosition({0.0f, 0.0f});
            setPositionMode(PositionMode::Absolute);
            setSize(LayoutSizeValue::fixed(size.width, size.height));
        }

    protected:
        void draw(SDL_Renderer *renderer) override
        {
            if (!renderer || opacity_ <= 0.0f)
                return;

            const uint8_t alpha = static_cast<uint8_t>(
                std::clamp(
                    static_cast<int>(std::lround(
                        static_cast<float>(color_.a) * opacity_)),
                    0,
                    255));

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_BLEND);

            SDL_SetRenderDrawColor(
                renderer,
                color_.r,
                color_.g,
                color_.b,
                alpha);

            const LayoutPosition position = getActualPosition();
            const LayoutSize size = getActualSize();

            SDL_FRect rect{
                position.x,
                position.y,
                size.width,
                size.height};

            SDL_RenderFillRect(renderer, &rect);
        }

    private:
        Color color_{0, 0, 0, 160};
        float opacity_ = 0.0f;
    };

    bool ModalManager::showModal(
        NodeTree &nodeTree,
        InputManager &input,
        Node &node,
        BackdropClickBehavior backdropClickBehavior)
    {
        if (!showModal(nodeTree, input, node))
            return false;

        if (!modals_.empty())
        {
            modals_.back().backdropClickBehavior =
                backdropClickBehavior;
        }

        ensureBackdrop(nodeTree);
        updateBackdropState();
        return true;
    }

    bool ModalManager::handlePointerDown(
        NodeTree &nodeTree,
        InputManager &input,
        const MousePosition &position,
        MouseButton button)
    {
        (void)button;

        Node *modal = topModalNode(nodeTree);

        if (!modal)
            return false;

        Node *target = nodeTree.hitTest(
            position.x,
            position.y,
            modal);

        if (target)
            return false;

        if (modals_.back().backdropClickBehavior ==
            BackdropClickBehavior::Close)
        {
            closeModal(nodeTree, input);
        }
        else
        {
            input.cancelPointerInteraction(nodeTree, position);
        }

        return true;
    }

    void ModalManager::setViewportSize(const LayoutSize &size) noexcept
    {
        viewportSize_ = size;

        if (backdropNode_)
            backdropNode_->setViewport(size);
    }

    void ModalManager::setBackdropColor(const Color &color) noexcept
    {
        backdropColor_ = color;

        if (backdropNode_)
        {
            backdropNode_->setBackdrop(
                backdropColor_,
                backdropOpacity_);
        }
    }

    Color ModalManager::getBackdropColor() const noexcept
    {
        return backdropColor_;
    }

    void ModalManager::setBackdropFadeDuration(float seconds) noexcept
    {
        backdropFadeDuration_ = std::max(0.0f, seconds);
    }

    float ModalManager::getBackdropFadeDuration() const noexcept
    {
        return backdropFadeDuration_;
    }

    void ModalManager::ensureBackdrop(NodeTree &nodeTree)
    {
        if (backdropNode_ && backdropId_ &&
            nodeTree.findNode(*backdropId_) == backdropNode_)
        {
            backdropNode_->setViewport(viewportSize_);
            backdropNode_->setBackdrop(
                backdropColor_,
                backdropOpacity_);
            return;
        }

        auto backdrop = std::make_unique<BackdropNode>();
        backdrop->setFocusable(false);
        backdrop->setCapturable(false);
        backdrop->setViewport(viewportSize_);
        backdrop->setBackdrop(
            backdropColor_,
            backdropOpacity_);

        Node *raw = nodeTree.attachOverlay(0, std::move(backdrop));

        if (!raw)
            return;

        backdropNode_ = static_cast<BackdropNode *>(raw);
        backdropId_ = raw->id();
    }

    void ModalManager::removeBackdrop(NodeTree &nodeTree) noexcept
    {
        if (!backdropNode_)
            return;

        Node *node = backdropNode_;

        backdropNode_ = nullptr;
        backdropId_.reset();

        nodeTree.removeOverlay(node);
    }

    void ModalManager::updateBackdropState() noexcept
    {
        backdropTargetOpacity_ = modals_.empty() ? 0.0f : 1.0f;

        if (backdropNode_)
        {
            backdropNode_->setBackdrop(
                backdropColor_,
                backdropOpacity_);
        }
    }

    void ModalManager::update(
        NodeTree &nodeTree,
        float dt) noexcept
    {
        backdropTargetOpacity_ = modals_.empty() ? 0.0f : 1.0f;

        if (backdropFadeDuration_ <= 0.0f)
        {
            backdropOpacity_ = backdropTargetOpacity_;
        }
        else
        {
            const float step = dt / backdropFadeDuration_;

            if (backdropOpacity_ < backdropTargetOpacity_)
            {
                backdropOpacity_ = std::min(
                    backdropTargetOpacity_,
                    backdropOpacity_ + step);
            }
            else if (backdropOpacity_ > backdropTargetOpacity_)
            {
                backdropOpacity_ = std::max(
                    backdropTargetOpacity_,
                    backdropOpacity_ - step);
            }
        }

        if (!modals_.empty())
        {
            ensureBackdrop(nodeTree);
        }
        else if (backdropOpacity_ <= 0.0f)
        {
            removeBackdrop(nodeTree);
        }

        if (backdropNode_)
        {
            backdropNode_->setViewport(viewportSize_);
            backdropNode_->setBackdrop(
                backdropColor_,
                backdropOpacity_);
        }
    }

    void ModalManager::clear(
        NodeTree &nodeTree,
        InputManager &input) noexcept
    {
        modals_.clear();
        input.cancelPointerInteraction(nodeTree);
        input.clearFocus(nodeTree);

        backdropTargetOpacity_ = 0.0f;
        backdropOpacity_ = 0.0f;
        removeBackdrop(nodeTree);
    }
}
