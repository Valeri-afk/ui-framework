#include "ui_framework/components/list.hpp"

#include <utility>

namespace ui
{
    List::List()
        : StackPanelNode(Orientation::Vertical)
    {
        setGap(0.0f);
        setMainAlignment(MainAxisAlignment::START);
        setCrossAlignment(CrossAxisAlignment::STRETCH);
    }

    ListItem *List::addItem(std::unique_ptr<ListItem> item, size_t index)
    {
        if (!item)
            return nullptr;

        ListItem *rawItem = item.get();
        if (add(std::move(item), index) != rawItem)
            return nullptr;

        return rawItem;
    }

    void List::removeItem(ListItem &item)
    {
        remove(item);
    }
}
