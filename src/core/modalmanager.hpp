#pragma once

#include <optional>
#include <vector>

#include "inputmanager.hpp"
#include "nodetree.hpp"
#include "ui_framework/core/panelnode.hpp"

namespace ui
{
    enum class BackdropClickBehavior
    {
        Consume,
        Close
    };

    class ModalManager
    {
    public:
        ModalManager();

        ModalManager(const ModalManager &) = delete;
        ModalManager &operator=(const ModalManager &) = delete;

        bool showModal(
            NodeTree &nodeTree,
            InputManager &input,
            Node &node,
            BackdropClickBehavior backdropClickBehavior =
                BackdropClickBehavior::Consume);

        bool closeModal(
            NodeTree &nodeTree,
            InputManager &input);

        bool handleKeyDown(
            NodeTree &nodeTree,
            InputManager &input,
            KeyCode key);

        bool handlePointerDown(
            NodeTree &nodeTree,
            InputManager &input,
            const MousePosition &position,
            MouseButton button);

        void update(float dt) noexcept;

        void drawBackdrop(
            SDL_Renderer *renderer,
            const LayoutSize &viewport) const noexcept;

        void setBackdropColor(const Color &color) noexcept;
        Color getBackdropColor() const noexcept;

        void setBackdropFadeDuration(float seconds) noexcept;
        float getBackdropFadeDuration() const noexcept;

        void clear(NodeTree &nodeTree, InputManager &input) noexcept;

        bool isModal(const Node *node) const noexcept;

        Node *topModalNode(NodeTree &nodeTree) const noexcept;
        const Node *topModalNode(const NodeTree &nodeTree) const noexcept;

        void sync(
            NodeTree &nodeTree,
            InputManager &input);

    private:
        struct ModalSession
        {
            Node::Id modalId{};
            std::optional<Node::Id> previousFocusId;
            BackdropClickBehavior backdropClickBehavior =
                BackdropClickBehavior::Consume;
        };

        std::vector<ModalSession> modals_;
        Color backdropColor_{0, 0, 0, 160};
        float backdropOpacity_ = 0.0f;
        float backdropTargetOpacity_ = 0.0f;
        float backdropFadeDuration_ = 0.15f;

        Node *findFirstFocusable(Node &node) const;
        Node *findFirstFocusableInTree(NodeTree &nodeTree) const;

        Node *findValidFocus(
            NodeTree &nodeTree,
            std::optional<Node::Id> preferredFocusId) const;

        bool isNodeUnder(
            const Node *node,
            const Node *ancestor) const noexcept;

        void restoreFocusAfterClose(
            NodeTree &nodeTree,
            InputManager &input,
            const ModalSession &session) const;

        void syncFocusForTopModal(
            NodeTree &nodeTree,
            InputManager &input) const;

        bool isLiveVisibleEnabledModal(
            NodeTree &nodeTree,
            const Node &node) const noexcept;

        bool eraseInvalidModalSession(
            NodeTree &nodeTree,
            InputManager &input,
            size_t index);

        void focusOrClear(
            NodeTree &nodeTree,
            InputManager &input,
            Node *focus) const;
    };

}