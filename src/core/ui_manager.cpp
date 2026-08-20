#include <algorithm>
#include <optional>
#include <utility>

#include "nodetree.hpp"
#include "inputmanager.hpp"
#include "modalmanager.hpp"
#include "layoutmanager.hpp"
#include "ui_framework/core/ui_manager.hpp"

namespace ui
{

    UIManager::UIManager()
        : nodeTree_(std::make_unique<NodeTree>()),
          inputManager_(std::make_unique<InputManager>()),
          modalManager_(std::make_unique<ModalManager>()),
          layoutManager_(std::make_unique<LayoutManager>())
    {
    }

    UIManager::~UIManager() = default;

    void UIManager::runFrame(
        float dt,
        SDL_Renderer *renderer)
    {
        if (!nodeTree_)
            return;

        if (layoutManager_ &&
            layoutManager_->syncViewportFromRenderer(renderer))
        {
            nodeTree_->requestFullLayout();
        }

        prepareForTreeOperation();
        update(dt);
        draw(renderer);
    }

    void UIManager::processEvent(
        const SDL_Event &sdlEvent)
    {
        if (!nodeTree_)
            return;

        prepareForTreeOperation();

        if (inputManager_ && modalManager_ &&
            sdlEvent.type == SDL_EVENT_KEY_DOWN &&
            convertSDLKeyCodeToKeyCode(sdlEvent.key.key) == KeyCode::ESCAPE)
        {
            if (modalManager_->handleKeyDown(
                    *nodeTree_,
                    *inputManager_,
                    KeyCode::ESCAPE))
            {
                prepareForTreeOperation();
                return;
            }
        }

        if (inputManager_ && modalManager_ &&
            sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            topModalNode())
        {
            const MousePosition position{
                sdlEvent.button.x,
                sdlEvent.button.y};

            const MouseButton button =
                static_cast<MouseButton>(sdlEvent.button.button);

            if (modalManager_->handlePointerDown(
                    *nodeTree_,
                    *inputManager_,
                    position,
                    button))
            {
                prepareForTreeOperation();
                return;
            }
        }

        if (inputManager_)
        {
            inputManager_->setModalRoot(topModalNode());

            inputManager_->processEvent(
                sdlEvent,
                *nodeTree_,
                topModalNode());

            inputManager_->setModalRoot(topModalNode());
        }

        prepareForTreeOperation();
    }

    void UIManager::update(float dt)
    {
        if (!nodeTree_)
            return;

        if (modalManager_)
            modalManager_->update(*nodeTree_, dt);

        nodeTree_->update(dt);

        if (layoutManager_)
            layoutManager_->processLayoutQueue(*nodeTree_);

        syncState();
    }

    void UIManager::draw(SDL_Renderer *renderer)
    {
        drawNodesForFrame(renderer);
    }

    Node *UIManager::attachRoot(
        size_t index,
        std::unique_ptr<Node> node)
    {
        return nodeTree_
                   ? nodeTree_->attachRoot(index, std::move(node))
                   : nullptr;
    }

    Node *UIManager::attachOverlay(
        size_t index,
        std::unique_ptr<Node> node)
    {
        return nodeTree_
                   ? nodeTree_->attachOverlay(index, std::move(node))
                   : nullptr;
    }

    void UIManager::removeRoot(Node *node)
    {
        if (nodeTree_)
            nodeTree_->removeRoot(node);
    }

    void UIManager::removeOverlay(Node *node)
    {
        if (nodeTree_)
            nodeTree_->removeOverlay(node);
    }

    bool UIManager::showModal(Node &node)
    {
        return showModal(node, BackdropClickBehavior::Consume);
    }

    bool UIManager::showModal(
        Node &node,
        BackdropClickBehavior behavior)
    {
        if (!nodeTree_ || !modalManager_ || !inputManager_)
            return false;

        prepareForTreeOperation();

        const bool shown = modalManager_->showModal(
            *nodeTree_,
            *inputManager_,
            node,
            behavior);

        if (shown)
            prepareForTreeOperation();

        return shown;
    }

    bool UIManager::closeModal()
    {
        if (!nodeTree_ || !modalManager_ || !inputManager_)
            return false;

        prepareForTreeOperation();

        const bool closed = modalManager_->closeModal(
            *nodeTree_,
            *inputManager_);

        if (closed)
            prepareForTreeOperation();

        return closed;
    }

    bool UIManager::isModal(const Node *node) const noexcept
    {
        if (!nodeTree_ || !modalManager_)
            return false;

        return modalManager_->isModal(node);
    }

    Node *UIManager::topModalNode() const noexcept
    {
        if (!nodeTree_ || !modalManager_)
            return nullptr;

        return modalManager_->topModalNode(*nodeTree_);
    }

    void UIManager::setBackdropColor(const Color &color) noexcept
    {
        if (modalManager_)
            modalManager_->setBackdropColor(color);
    }

    Color UIManager::getBackdropColor() const noexcept
    {
        return modalManager_ ? modalManager_->getBackdropColor() : Color{};
    }

    void UIManager::setBackdropFadeDuration(float seconds) noexcept
    {
        if (modalManager_)
            modalManager_->setBackdropFadeDuration(seconds);
    }

    float UIManager::getBackdropFadeDuration() const noexcept
    {
        return modalManager_ ? modalManager_->getBackdropFadeDuration() : 0.0f;
    }

    void UIManager::prepareForTreeOperation()
    {
        if (!nodeTree_)
            return;

        applyMutationQueue();
        if (layoutManager_)
            layoutManager_->processLayoutQueue(*nodeTree_);
        syncState();
    }

    void UIManager::syncModalInputState()
    {
        if (inputManager_)
            inputManager_->setModalRoot(topModalNode());
    }

    void UIManager::drawNodesForFrame(
        SDL_Renderer *renderer)
    {
        if (!renderer || !nodeTree_)
            return;

        std::optional<Node::Id> topModalId;

        if (modalManager_)
        {
            if (Node *topModal = modalManager_->topModalNode(*nodeTree_))
                topModalId = topModal->id();
        }

        nodeTree_->draw(renderer, topModalId);
        syncState();
    }

    void UIManager::applyMutationQueue()
    {
        if (nodeTree_)
            nodeTree_->flushMutationQueue();
    }

    void UIManager::syncState()
    {
        if (!nodeTree_ || !inputManager_)
            return;

        if (modalManager_)
            modalManager_->sync(*nodeTree_, *inputManager_);

        inputManager_->syncState(*nodeTree_);
        syncModalInputState();
    }

}
