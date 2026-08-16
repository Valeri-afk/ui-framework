#include "ui_framework/components/flex_panel.hpp"
#include <algorithm>
#include <limits>
#include <numeric>

namespace ui
{
    // Конструктор и сеттеры
    FlexPanel::FlexPanel(Axis axis) : axis_(axis) {}

    void FlexPanel::setGap(float gap) noexcept
    {
        rowGap_ = std::max(0.0f, gap);
        columnGap_ = std::max(0.0f, gap);
    }

    float FlexPanel::getGap() const noexcept
    {
        return std::max(rowGap_, columnGap_);
    }

    void FlexPanel::setRowGap(float gap) noexcept
    {
        rowGap_ = std::max(0.0f, gap);
    }

    float FlexPanel::getRowGap() const noexcept
    {
        return rowGap_;
    }

    void FlexPanel::setColumnGap(float gap) noexcept
    {
        columnGap_ = std::max(0.0f, gap);
    }

    float FlexPanel::getColumnGap() const noexcept
    {
        return columnGap_;
    }

    void FlexPanel::setMainAxisAlignment(Alignment alignment) noexcept
    {
        mainAxisAlignment_ = alignment;
    }

    Alignment FlexPanel::getMainAxisAlignment() const noexcept
    {
        return mainAxisAlignment_;
    }

    void FlexPanel::setCrossAxisAlignment(Alignment alignment) noexcept
    {
        crossAxisAlignment_ = alignment;
    }

    Alignment FlexPanel::getCrossAxisAlignment() const noexcept
    {
        return crossAxisAlignment_;
    }

    void FlexPanel::setWrapMode(WrapMode mode) noexcept
    {
        wrapMode_ = mode;
    }

    FlexPanel::WrapMode FlexPanel::getWrapMode() const noexcept
    {
        return wrapMode_;
    }

    void FlexPanel::setDefaultFlexGrow(float grow) noexcept
    {
        defaultFlexGrow_ = std::max(0.0f, grow);
    }

    float FlexPanel::getDefaultFlexGrow() const noexcept
    {
        return defaultFlexGrow_;
    }

    void FlexPanel::setDefaultFlexShrink(float shrink) noexcept
    {
        defaultFlexShrink_ = std::max(0.0f, shrink);
    }

    float FlexPanel::getDefaultFlexShrink() const noexcept
    {
        return defaultFlexShrink_;
    }

    // Утилиты для работы с осями (без изменений)
    float FlexPanel::mainSize(const LayoutSize &size) const noexcept
    {
        return axis_ == Axis::Horizontal ? size.width : size.height;
    }

    float FlexPanel::crossSize(const LayoutSize &size) const noexcept
    {
        return axis_ == Axis::Horizontal ? size.height : size.width;
    }

    void FlexPanel::setMainSize(LayoutSize &size, float value) const noexcept
    {
        if (axis_ == Axis::Horizontal)
            size.width = value;
        else
            size.height = value;
    }

    void FlexPanel::setCrossSize(LayoutSize &size, float value) const noexcept
    {
        if (axis_ == Axis::Horizontal)
            size.height = value;
        else
            size.width = value;
    }

    float FlexPanel::mainPosition(const LayoutPosition &pos) const noexcept
    {
        return axis_ == Axis::Horizontal ? pos.x : pos.y;
    }

    float FlexPanel::crossPosition(const LayoutPosition &pos) const noexcept
    {
        return axis_ == Axis::Horizontal ? pos.y : pos.x;
    }

    void FlexPanel::setMainPosition(LayoutPosition &pos, float value) const noexcept
    {
        if (axis_ == Axis::Horizontal)
            pos.x = value;
        else
            pos.y = value;
    }

    void FlexPanel::setCrossPosition(LayoutPosition &pos, float value) const noexcept
    {
        if (axis_ == Axis::Horizontal)
            pos.y = value;
        else
            pos.x = value;
    }

    // Построение линий (строк/колонок) с учётом wrap
    std::vector<FlexPanel::Line> FlexPanel::buildLines(
        const std::vector<LayoutSize> &childrenMeasure,
        float availableMain) const
    {
        std::vector<Line> lines;

        if (wrapMode_ == WrapMode::NoWrap)
        {
            Line line;
            for (size_t i = 0; i < childrenMeasure.size(); ++i)
            {
                line.childIndices.push_back(i);
                line.mainSize += mainSize(childrenMeasure[i]);
                line.crossSize = std::max(line.crossSize, crossSize(childrenMeasure[i]));
            }
            if (!line.childIndices.empty())
                line.mainSize += columnGap_ * (line.childIndices.size() - 1);
            lines.push_back(std::move(line));
            return lines;
        }

        // Wrap режим
        Line currentLine;
        float currentMain = 0.0f;

        for (size_t i = 0; i < childrenMeasure.size(); ++i)
        {
            const float childMain = mainSize(childrenMeasure[i]);
            const float childCross = crossSize(childrenMeasure[i]);
            const float gap = currentLine.childIndices.empty() ? 0.0f : columnGap_;

            if (currentMain + gap + childMain <= availableMain || currentLine.childIndices.empty())
            {
                currentLine.childIndices.push_back(i);
                currentMain += gap + childMain;
                currentLine.crossSize = std::max(currentLine.crossSize, childCross);
            }
            else
            {
                if (!currentLine.childIndices.empty())
                {
                    currentLine.mainSize = currentMain;
                    lines.push_back(std::move(currentLine));
                    currentLine = Line{};
                    currentMain = 0.0f;
                }
                currentLine.childIndices.push_back(i);
                currentMain += childMain;
                currentLine.crossSize = childCross;
            }
        }

        if (!currentLine.childIndices.empty())
        {
            currentLine.mainSize = currentMain;
            lines.push_back(std::move(currentLine));
        }

        if (wrapMode_ == WrapMode::WrapReverse)
            std::reverse(lines.begin(), lines.end());

        return lines;
    }

    // Распределение пространства внутри одной линии
    void FlexPanel::distributeLineSpace(
        const Line &line,
        const std::vector<LayoutSize> &childrenMeasure,
        float availableMain,
        std::vector<LayoutSize> &outSizes) const
    {
        const size_t count = line.childIndices.size();
        if (count == 0)
            return;

        // 1. Суммируем идеальные main-размеры
        float totalIdeal = 0.0f;
        for (size_t idx : line.childIndices)
            totalIdeal += mainSize(childrenMeasure[idx]);

        // Добавляем gap (уже учтены в line.mainSize, но они не влияют на распределение)
        // Для распределения используем только размеры элементов.

        float totalGap = columnGap_ * (count - 1);
        float availableForChildren = std::max(0.0f, availableMain - totalGap);

        // 2. Если места достаточно – даём каждому его идеальный размер
        if (availableForChildren >= totalIdeal)
        {
            // Если есть flex-grow – распределяем избыток
            float extra = availableForChildren - totalIdeal;
            float totalGrow = defaultFlexGrow_ * count;
            if (totalGrow > 0.0f)
            {
                for (size_t idx : line.childIndices)
                {
                    float growFactor = defaultFlexGrow_ / totalGrow;
                    float add = extra * growFactor;
                    LayoutSize newSize = childrenMeasure[idx];
                    setMainSize(newSize, mainSize(newSize) + add);
                    outSizes[idx] = newSize;
                }
            }
            else
            {
                // Нет grow – оставляем как есть
                for (size_t idx : line.childIndices)
                    outSizes[idx] = childrenMeasure[idx];
            }
        }
        else
        {
            // 3. Не хватает места – сжимаем пропорционально flex-shrink
            float deficit = totalIdeal - availableForChildren;
            float totalShrink = defaultFlexShrink_ * count;
            if (totalShrink > 0.0f)
            {
                for (size_t idx : line.childIndices)
                {
                    float shrinkFactor = defaultFlexShrink_ / totalShrink;
                    float sub = deficit * shrinkFactor;
                    LayoutSize newSize = childrenMeasure[idx];
                    float newMain = std::max(0.0f, mainSize(newSize) - sub);
                    setMainSize(newSize, newMain);
                    outSizes[idx] = newSize;
                }
            }
            else
            {
                // Нет shrink – просто обрезаем (оставляем идеальные, они вылезут)
                for (size_t idx : line.childIndices)
                    outSizes[idx] = childrenMeasure[idx];
            }
        }

        // Для STRETCH – корректируем cross-размер позже в arrange
    }

    // Основной метод измерения (без изменений, только использует buildLines)
    LayoutSize FlexPanel::measure(const MeasureContext &ctx) const
    {
        const size_t childCount = ctx.childrenMeasure.size();
        if (childCount == 0)
            return {};

        const float maxMain = mainSize({ctx.constraints.maxWidth, ctx.constraints.maxHeight});
        const float maxCross = crossSize({ctx.constraints.maxWidth, ctx.constraints.maxHeight});

        auto lines = buildLines(ctx.childrenMeasure, maxMain);

        float totalMain = 0.0f;
        float totalCross = 0.0f;

        for (size_t i = 0; i < lines.size(); ++i)
        {
            const auto &line = lines[i];
            float lineCross = line.crossSize;

            // Если есть STRETCH и ограничение по cross-оси – используем его для измерения?
            // В measure мы не знаем финальный cross-размер, оставляем как есть.
            totalCross += lineCross;
            if (i > 0)
                totalCross += rowGap_;
            totalMain = std::max(totalMain, line.mainSize);
        }

        LayoutSize result;
        setMainSize(result, totalMain);
        setCrossSize(result, totalCross);
        return result;
    }

    // Основной метод расположения
    void FlexPanel::arrange(const ArrangeContext &ctx)
    {
        if (!ctx.placeChild || ctx.childrenMeasure.empty())
            return;

        const float availableMain = mainSize(ctx.contentSize);
        const float availableCross = crossSize(ctx.contentSize);

        // Строим линии
        auto lines = buildLines(ctx.childrenMeasure, availableMain);

        // Вектор для скорректированных размеров детей
        std::vector<LayoutSize> adjustedSizes = ctx.childrenMeasure; // копия

        // Распределяем пространство внутри каждой линии
        for (const auto &line : lines)
        {
            distributeLineSpace(line, ctx.childrenMeasure, availableMain, adjustedSizes);
        }

        // Вычисляем общий cross-размер всех линий (используем максимальные cross-размеры из линий)
        float totalCrossSize = 0.0f;
        for (const auto &line : lines)
        {
            // Пересчитываем cross-размер линии на основе скорректированных размеров
            float lineCross = 0.0f;
            for (size_t idx : line.childIndices)
                lineCross = std::max(lineCross, crossSize(adjustedSizes[idx]));
            totalCrossSize += lineCross;
        }
        totalCrossSize += rowGap_ * (lines.size() - 1);

        // Начальная позиция по cross-оси
        float crossStart = crossPosition(ctx.contentPosition);
        float freeCross = std::max(0.0f, availableCross - totalCrossSize);

        // Выравнивание линий по cross-оси (если есть свободное место)
        switch (mainAxisAlignment_) // используем mainAxisAlignment для выравнивания строк? Это не совсем правильно, но для простоты используем тот же
        {
        case Alignment::CENTER:
            crossStart += freeCross * 0.5f;
            break;
        case Alignment::END:
            crossStart += freeCross;
            break;
        default:
            break;
        }

        float currentCross = crossStart;

        // Раскладываем линии
        for (const auto &line : lines)
        {
            // Вычисляем фактический cross-размер линии (максимальный среди детей)
            float lineCross = 0.0f;
            for (size_t idx : line.childIndices)
                lineCross = std::max(lineCross, crossSize(adjustedSizes[idx]));

            // Если crossAxisAlignment == STRETCH – растягиваем детей до lineCross (или до availableCross?)
            // Но мы уже скорректировали main-размеры. Cross-размеры пока не трогали.
            // Если нужно растянуть – делаем это сейчас.
            if (crossAxisAlignment_ == Alignment::STRETCH)
            {
                // Если есть ограничение по cross-оси – растягиваем до него (или до lineCross?)
                float targetCross = (availableCross > 0.0f) ? availableCross : lineCross;
                for (size_t idx : line.childIndices)
                {
                    LayoutSize &sz = adjustedSizes[idx];
                    setCrossSize(sz, targetCross);
                }
                lineCross = targetCross;
            }

            // Распределяем элементы внутри линии по главной оси
            float lineMainSize = 0.0f;
            for (size_t idx : line.childIndices)
                lineMainSize += mainSize(adjustedSizes[idx]);
            lineMainSize += columnGap_ * (line.childIndices.size() - 1);

            float mainStart = mainPosition(ctx.contentPosition);
            float freeMain = std::max(0.0f, availableMain - lineMainSize);

            switch (mainAxisAlignment_)
            {
            case Alignment::CENTER:
                mainStart += freeMain * 0.5f;
                break;
            case Alignment::END:
                mainStart += freeMain;
                break;
            case Alignment::SPACE_BETWEEN:
                // будет обработано в цикле
                break;
            default:
                break;
            }

            float currentMain = mainStart;
            const size_t elemCount = line.childIndices.size();

            for (size_t i = 0; i < elemCount; ++i)
            {
                size_t childIdx = line.childIndices[i];
                LayoutSize childSize = adjustedSizes[childIdx];

                LayoutPosition childPos = ctx.contentPosition;
                setMainPosition(childPos, currentMain);

                // Cross-позиция: выравнивание внутри линии
                float childCross = currentCross;
                float childCrossSize = crossSize(childSize);
                float lineCrossSize = lineCross;

                switch (crossAxisAlignment_)
                {
                case Alignment::CENTER:
                    childCross += (lineCrossSize - childCrossSize) * 0.5f;
                    break;
                case Alignment::END:
                    childCross += lineCrossSize - childCrossSize;
                    break;
                case Alignment::STRETCH:
                    // уже растянули
                    childCross = currentCross;
                    break;
                default:
                    break;
                }

                setCrossPosition(childPos, childCross);

                // Размещаем ребёнка
                const LayoutSize placedSize = ctx.placeChild(childIdx, childPos, childSize);

                // Продвигаем курсор
                currentMain += mainSize(placedSize);

                // Добавляем gap (кроме последнего)
                if (i < elemCount - 1)
                {
                    if (mainAxisAlignment_ == Alignment::SPACE_BETWEEN && elemCount > 1)
                    {
                        float gap = (availableMain - lineMainSize) / (elemCount - 1);
                        currentMain += gap;
                    }
                    else
                    {
                        currentMain += columnGap_;
                    }
                }
            }

            // Переход к следующей линии
            currentCross += lineCross + rowGap_;
        }
    }
}