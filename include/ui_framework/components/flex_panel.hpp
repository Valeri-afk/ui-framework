#pragma once

#include "ui_framework/components/component.hpp"
#include "ui_framework/types.hpp"
#include <vector>

namespace ui
{
    class FlexPanel : public Component
    {
    public:
        enum class Axis
        {
            Horizontal,
            Vertical
        };
        enum class WrapMode
        {
            NoWrap,
            Wrap,
            WrapReverse
        };

        explicit FlexPanel(Axis axis);

        // Основные свойства
        void setGap(float gap) noexcept;
        float getGap() const noexcept;

        void setRowGap(float gap) noexcept;
        float getRowGap() const noexcept;

        void setColumnGap(float gap) noexcept;
        float getColumnGap() const noexcept;

        void setMainAxisAlignment(Alignment alignment) noexcept;
        Alignment getMainAxisAlignment() const noexcept;

        void setCrossAxisAlignment(Alignment alignment) noexcept;
        Alignment getCrossAxisAlignment() const noexcept;

        void setWrapMode(WrapMode mode) noexcept;
        WrapMode getWrapMode() const noexcept;

        // Flex-коэффициенты (общие для всех детей)
        void setDefaultFlexGrow(float grow) noexcept;
        float getDefaultFlexGrow() const noexcept;

        void setDefaultFlexShrink(float shrink) noexcept;
        float getDefaultFlexShrink() const noexcept;

        LayoutSize measure(const MeasureContext &ctx) const override;
        void arrange(const ArrangeContext &ctx) override;

    private:
        Axis axis_;
        float rowGap_ = 0.0f;
        float columnGap_ = 0.0f;
        Alignment mainAxisAlignment_ = Alignment::START;
        Alignment crossAxisAlignment_ = Alignment::START;
        WrapMode wrapMode_ = WrapMode::NoWrap;

        float defaultFlexGrow_ = 0.0f;   // 0 – не растягивается
        float defaultFlexShrink_ = 1.0f; // 1 – нормальное сжатие

        struct Line
        {
            std::vector<size_t> childIndices;
            float mainSize = 0.0f;  // сумма идеальных main-размеров + gap
            float crossSize = 0.0f; // максимальный cross-размер
        };

        // Утилиты для работы с осями
        float mainSize(const LayoutSize &size) const noexcept;
        float crossSize(const LayoutSize &size) const noexcept;
        void setMainSize(LayoutSize &size, float value) const noexcept;
        void setCrossSize(LayoutSize &size, float value) const noexcept;

        float mainPosition(const LayoutPosition &pos) const noexcept;
        float crossPosition(const LayoutPosition &pos) const noexcept;
        void setMainPosition(LayoutPosition &pos, float value) const noexcept;
        void setCrossPosition(LayoutPosition &pos, float value) const noexcept;

        // Вспомогательные методы
        std::vector<Line> buildLines(
            const std::vector<LayoutSize> &childrenMeasure,
            float availableMain) const;

        // Распределение пространства внутри линии
        void distributeLineSpace(
            const Line &line,
            const std::vector<LayoutSize> &childrenMeasure,
            float availableMain,
            std::vector<LayoutSize> &outSizes) const;
    };

    class Row : public FlexPanel
    {
    public:
        Row() : FlexPanel(Axis::Horizontal) {}
    };
    class Column : public FlexPanel
    {
    public:
        Column() : FlexPanel(Axis::Vertical) {}
    };
}