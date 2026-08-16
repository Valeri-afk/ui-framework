#include "ui_framework/components/paper.hpp"

namespace ui
{
    Paper::Paper() : Widget()
    {
        setPreferredSize({400, 120});
        setBackgroundColor(Colors::white);
        setPadding({10, 10, 10, 10});
    }

    Paper::Paper(const LayoutPosition &position, const LayoutSize &size) : Paper()
    {
        setPreferredPosition(position);
        setPreferredSize(size);
    }

}