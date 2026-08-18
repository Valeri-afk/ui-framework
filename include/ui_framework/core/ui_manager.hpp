#pragma once

#include <cstddef>
#include <memory>

#include <SDL3/SDL.h>

#include "ui_framework/core/panelnode.hpp"
#include "ui_framework/types.hpp"

namespace ui
{

    class NodeTree;
    class InputManager;
    class ModalManager;
    class LayoutManager;

    class UIManager
    {
    public:
        UIManager();
        ~UIManager();

        UIManager(const UIManager &) = delete;
        UIManager &operator=(const UIManager &) = delete;

        void runFrame(float dt, SDL_Renderer *renderer);
        void processEvent(const SDL_Event &sdlEvent);

        Node *attachRoot(size_t index, std::unique_ptr<Node> node);
        Node *attachOverlay(size_t index, std::unique_ptr<Node> node);

        void removeRoot(Node *node);
        void removeOverlay(Node *node);

        void setViewportSize(const LayoutSize &size) noexcept;
        LayoutSize getViewportSize() const noexcept;

        bool showModal(Node &node);
        bool closeModal();

        bool isModal(const Node *node) const noexcept;
        Node *topModalNode() const noexcept;

    private:
        void update(float dt);
        void draw(SDL_Renderer *renderer);

        void prepareForTreeOperation();
        void syncModalInputState();
        void drawNodesForFrame(SDL_Renderer *renderer);

        void applyMutationQueue();
        void syncState();

    private:
        std::unique_ptr<NodeTree> nodeTree_;
        std::unique_ptr<InputManager> inputManager_;
        std::unique_ptr<ModalManager> modalManager_;
        std::unique_ptr<LayoutManager> layoutManager_;
    };

}