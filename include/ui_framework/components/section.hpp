#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

#include "ui_framework/components/button.hpp"
#include "ui_framework/core/stackpanelnode.hpp"

namespace ui
{
    class Section : public StackPanelNode
    {
    public:
        using ExpansionCallback = std::function<void(Section &, bool)>;

        Section();
        ~Section() override = default;

        void setTitle(std::string title);
        const std::string &getTitle() const noexcept;

        void setExpanded(bool expanded);
        bool isExpanded() const noexcept;
        void toggle();

        Node *addContent(std::unique_ptr<Node> child, size_t index = static_cast<size_t>(-1));
        void removeContent(Node &child);
        size_t contentCount() const noexcept;
        Node *getContentAt(size_t index) noexcept;

        void setOnExpansionChanged(ExpansionCallback callback);

    protected:
        void onExpansionChanged();

    private:
        Button *header_ = nullptr;
        StackPanelNode *content_ = nullptr;
        bool expanded_ = false;
        ExpansionCallback onExpansionChanged_;
    };
}
