#include "ui_framework/core/gridnode.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float kInfinity = std::numeric_limits<float>::max();

    float sanitizeFinite(float value) noexcept
    {
        return std::isfinite(value) ? std::max(0.0f, value) : 0.0f;
    }

    float sanitizeFr(float value) noexcept
    {
        return std::isfinite(value) && value > 0.0f ? value : 0.0f;
    }

    size_t safeAddIndex(size_t value, size_t addend) noexcept
    {
        if (addend > std::numeric_limits<size_t>::max() - value)
            return std::numeric_limits<size_t>::max();

        return value + addend;
    }
}

namespace ui
{
    GridNode::GridNode()
    {
        rows_.push_back(TrackDefinition::autoTrack());
        columns_.push_back(TrackDefinition::autoTrack());
    }

    void GridNode::setRows(std::vector<TrackDefinition> rows)
    {
        for (auto &row : rows)
        {
            row = sanitizeTrack(row);
        }

        if (rows.empty())
        {
            rows.push_back(TrackDefinition::autoTrack());
        }

        deferLayoutMutation(
            [rows = std::move(rows)](Node &node) mutable
            {
                static_cast<GridNode &>(node).rows_ = std::move(rows);
            });
    }

    void GridNode::setColumns(std::vector<TrackDefinition> columns)
    {
        for (auto &column : columns)
        {
            column = sanitizeTrack(column);
        }

        if (columns.empty())
        {
            columns.push_back(TrackDefinition::autoTrack());
        }

        deferLayoutMutation(
            [columns = std::move(columns)](Node &node) mutable
            {
                static_cast<GridNode &>(node).columns_ = std::move(columns);
            });
    }

    const std::vector<GridNode::TrackDefinition> &
    GridNode::getRows() const noexcept
    {
        return rows_;
    }

    const std::vector<GridNode::TrackDefinition> &
    GridNode::getColumns() const noexcept
    {
        return columns_;
    }

    void GridNode::setPlacement(
        Node &child,
        const GridPlacement &placement)
    {
        const Node::Id childId = child.id();
        const GridPlacement safePlacement =
            sanitizePlacement(placement);

        deferLayoutMutation(
            [childId, safePlacement](Node &node)
            {
                GridNode &grid = static_cast<GridNode &>(node);

                for (size_t i = 0; i < grid.placementIds_.size(); ++i)
                {
                    if (grid.placementIds_[i] == childId)
                    {
                        grid.placements_[i] = safePlacement;
                        return;
                    }
                }

                grid.placementIds_.push_back(childId);
                grid.placements_.push_back(safePlacement);
            });
    }

    GridNode::GridPlacement GridNode::getPlacement(
        const Node &child) const noexcept
    {
        for (size_t i = 0; i < placementIds_.size(); ++i)
        {
            if (placementIds_[i] == child.id())
            {
                return placements_[i];
            }
        }

        return {};
    }

    LayoutSize GridNode::measure(MeasureContext &ctx)
    {
        if (!ctx.measureChild)
            return {};

        const size_t rowTotal = rowCount();
        const size_t columnTotal = columnCount();

        std::vector<float> rowSizes(rowTotal, 0.0f);
        std::vector<float> columnSizes(columnTotal, 0.0f);

        size_t visibleIndex = 0;

        for (size_t childIndex = 0; childIndex < childCount(); ++childIndex)
        {
            Node *child = getChildAt(childIndex);

            if (!child || !child->isVisible())
                continue;

            const GridPlacement placement = getPlacement(*child);

            const size_t row = std::min(placement.row, rowTotal - 1);
            const size_t column =
                std::min(placement.column, columnTotal - 1);
            const size_t rowEnd =
                std::min(
                    safeAddIndex(row, placement.rowSpan),
                    rowTotal);
            const size_t columnEnd =
                std::min(
                    safeAddIndex(column, placement.columnSpan),
                    columnTotal);

            LayoutSize childAvailable = ctx.availableSize;

            if (!std::isfinite(childAvailable.width))
                childAvailable.width = kInfinity;

            if (!std::isfinite(childAvailable.height))
                childAvailable.height = kInfinity;

            const LayoutSize childSize =
                ctx.measureChild(visibleIndex, childAvailable);

            const size_t rowSpan = std::max<size_t>(1, rowEnd - row);
            const size_t columnSpan =
                std::max<size_t>(1, columnEnd - column);

            if (rowSpan == 1)
            {
                rowSizes[row] =
                    std::max(rowSizes[row],
                             sanitizeFinite(childSize.height));
            }

            if (columnSpan == 1)
            {
                columnSizes[column] =
                    std::max(columnSizes[column],
                             sanitizeFinite(childSize.width));
            }

            ++visibleIndex;
        }

        for (size_t i = 0; i < rows_.size() && i < rowSizes.size(); ++i)
        {
            if (rows_[i].type == TrackType::Fixed)
            {
                rowSizes[i] = sanitizeFinite(rows_[i].value);
            }
        }

        for (size_t i = 0;
                  i < columns_.size() && i < columnSizes.size();
                  ++i)
        {
            if (columns_[i].type == TrackType::Fixed)
            {
                columnSizes[i] = sanitizeFinite(columns_[i].value);
            }
        }

        LayoutSize desired{};

        for (float value : rowSizes)
            desired.height += value;

        for (float value : columnSizes)
            desired.width += value;

        return desired;
    }

    void GridNode::arrange(ArrangeContext &ctx)
    {
        if (!ctx.placeChild)
            return;

        const size_t rowTotal = rowCount();
        const size_t columnTotal = columnCount();

        std::vector<float> rowSizes(rowTotal, 0.0f);
        std::vector<float> columnSizes(columnTotal, 0.0f);

        float fixedAndAutoHeight = 0.0f;
        float fixedAndAutoWidth = 0.0f;
        float totalFrRows = 0.0f;
        float totalFrColumns = 0.0f;

        for (size_t i = 0; i < rowTotal; ++i)
        {
            const TrackDefinition definition =
                i < rows_.size()
                    ? rows_[i]
                    : TrackDefinition::autoTrack();

            switch (definition.type)
            {
            case TrackType::Fixed:
                rowSizes[i] = sanitizeFinite(definition.value);
                fixedAndAutoHeight += rowSizes[i];
                break;
            case TrackType::Auto:
                rowSizes[i] = 0.0f;
                fixedAndAutoHeight += rowSizes[i];
                break;
            case TrackType::Fr:
                totalFrRows += sanitizeFr(definition.value);
                break;
            }
        }

        for (size_t i = 0; i < columnTotal; ++i)
        {
            const TrackDefinition definition =
                i < columns_.size()
                    ? columns_[i]
                    : TrackDefinition::autoTrack();

            switch (definition.type)
            {
            case TrackType::Fixed:
                columnSizes[i] = sanitizeFinite(definition.value);
                fixedAndAutoWidth += columnSizes[i];
                break;
            case TrackType::Auto:
                columnSizes[i] = 0.0f;
                fixedAndAutoWidth += columnSizes[i];
                break;
            case TrackType::Fr:
                totalFrColumns += sanitizeFr(definition.value);
                break;
            }
        }

        size_t visibleIndex = 0;
        for (size_t childIndex = 0; childIndex < childCount(); ++childIndex)
        {
            Node *child = getChildAt(childIndex);

            if (!child || !child->isVisible())
                continue;

            const GridPlacement placement = getPlacement(*child);

            const size_t row = std::min(placement.row, rowTotal - 1);
            const size_t column =
                std::min(placement.column, columnTotal - 1);
            const size_t rowEnd =
                std::min(
                    safeAddIndex(row, placement.rowSpan),
                    rowTotal);
            const size_t columnEnd =
                std::min(
                    safeAddIndex(column, placement.columnSpan),
                    columnTotal);

            const size_t rowSpan = std::max<size_t>(1, rowEnd - row);
            const size_t columnSpan =
                std::max<size_t>(1, columnEnd - column);

            const LayoutSize desired = child->getDesiredSize();

            if (rowSpan == 1 &&
                rows_[row].type == TrackType::Auto)
            {
                rowSizes[row] =
                    std::max(rowSizes[row],
                             sanitizeFinite(desired.height));
            }

            if (columnSpan == 1 &&
                columns_[column].type == TrackType::Auto)
            {
                columnSizes[column] =
                    std::max(columnSizes[column],
                             sanitizeFinite(desired.width));
            }

            ++visibleIndex;
        }

        const float remainingWidth =
            std::max(0.0f, ctx.contentSize.width - fixedAndAutoWidth);
        const float remainingHeight =
            std::max(0.0f, ctx.contentSize.height - fixedAndAutoHeight);

        if (totalFrColumns > 0.0f)
        {
            for (size_t i = 0; i < columnTotal; ++i)
            {
                const TrackDefinition definition =
                    i < columns_.size()
                        ? columns_[i]
                        : TrackDefinition::autoTrack();

                if (definition.type == TrackType::Fr)
                {
                    columnSizes[i] =
                        remainingWidth *
                        (sanitizeFr(definition.value) /
                         totalFrColumns);
                }
            }
        }

        if (totalFrRows > 0.0f)
        {
            for (size_t i = 0; i < rowTotal; ++i)
            {
                const TrackDefinition definition =
                    i < rows_.size()
                        ? rows_[i]
                        : TrackDefinition::autoTrack();

                if (definition.type == TrackType::Fr)
                {
                    rowSizes[i] =
                        remainingHeight *
                        (sanitizeFr(definition.value) /
                         totalFrRows);
                }
            }
        }

        std::vector<float> xOffsets(columnTotal + 1, 0.0f);
        std::vector<float> yOffsets(rowTotal + 1, 0.0f);

        for (size_t i = 0; i < columnTotal; ++i)
            xOffsets[i + 1] = xOffsets[i] + columnSizes[i];

        for (size_t i = 0; i < rowTotal; ++i)
            yOffsets[i + 1] = yOffsets[i] + rowSizes[i];

        visibleIndex = 0;
        for (size_t childIndex = 0; childIndex < childCount(); ++childIndex)
        {
            Node *child = getChildAt(childIndex);

            if (!child || !child->isVisible())
                continue;

            const GridPlacement placement = getPlacement(*child);

            const size_t row = std::min(placement.row, rowTotal - 1);
            const size_t column =
                std::min(placement.column, columnTotal - 1);
            const size_t rowEnd =
                std::min(
                    safeAddIndex(row, placement.rowSpan),
                    rowTotal);
            const size_t columnEnd =
                std::min(
                    safeAddIndex(column, placement.columnSpan),
                    columnTotal);

            const LayoutPosition position{
                ctx.contentPosition.x + xOffsets[column],
                ctx.contentPosition.y + yOffsets[row]};

            const LayoutSize size{
                xOffsets[columnEnd] - xOffsets[column],
                yOffsets[rowEnd] - yOffsets[row]};

            ctx.placeChild(visibleIndex, position, size);
            ++visibleIndex;
        }
    }

    GridNode::TrackDefinition GridNode::sanitizeTrack(
        TrackDefinition track) noexcept
    {
        switch (track.type)
        {
        case TrackType::Fixed:
            track.value = sanitizeFinite(track.value);
            break;
        case TrackType::Auto:
            track.value = 0.0f;
            break;
        case TrackType::Fr:
            track.value = sanitizeFr(track.value);
            break;
        }

        return track;
    }

    GridNode::GridPlacement GridNode::sanitizePlacement(
        GridPlacement placement) noexcept
    {
        placement.rowSpan = std::max<size_t>(1, placement.rowSpan);
        placement.columnSpan = std::max<size_t>(1, placement.columnSpan);
        return placement;
    }

    size_t GridNode::rowCount() const noexcept
    {
        size_t count = std::max<size_t>(1, rows_.size());

        for (const auto &placement : placements_)
        {
            count = std::max(
                count,
                safeAddIndex(placement.row, placement.rowSpan));
        }

        return count;
    }

    size_t GridNode::columnCount() const noexcept
    {
        size_t count = std::max<size_t>(1, columns_.size());

        for (const auto &placement : placements_)
        {
            count = std::max(
                count,
                safeAddIndex(placement.column, placement.columnSpan));
        }

        return count;
    }
}
