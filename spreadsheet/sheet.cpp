#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

constexpr static char COL_DELIMITER = '\t';
constexpr static char ROW_DELIMITER = '\n';

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    CheckPosition(pos);

    auto new_cell = std::make_unique<Cell>(*this, std::move(text));

    if(FindCyclicDeps(pos, *new_cell)) {
        throw CircularDependencyException("Circular dependency");
    }

    MaybeIncreaseSizeToIncludePosition(pos);

    auto& old_cell = cells_[pos.row][pos.col];
    if(old_cell) {
        for(auto child_pos: old_cell->GetChildren()) {
            RemoveParent(child_pos, old_cell.get());
        }
        new_cell->SetParents(std::move(old_cell->GetParents()));
    }
    for(auto child_pos: new_cell->GetChildren()) {
        GetOrCreateConcreteCell(child_pos).AddParent(new_cell.get());
    }
    InvalidateCache(*new_cell);
    if(!old_cell || old_cell->IsEmpty()) {
        ++row_cells_count_[pos.row];
        ++col_cells_count_[pos.col];
    }
    old_cell = std::move(new_cell);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    CheckPosition(pos);
    return GetConcreteCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    CheckPosition(pos);
    return GetConcreteCell(pos);
}

void Sheet::ClearCell(Position pos) {
    CheckPosition(pos);
    if(!CheckSize(pos)) {
        return;
    }
    auto& old_cell = cells_[pos.row][pos.col];
    if(!old_cell || old_cell->IsEmpty()) {
        return;
    }

    for(auto child_pos: old_cell->GetChildren()) {
        RemoveParent(child_pos, old_cell.get());
    }

    if(!old_cell->GetParents().empty())
    {
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->SetParents(std::move(old_cell->GetParents()));
        old_cell = std::move(new_cell);
    } else {
        old_cell.reset();
    }

    if(--row_cells_count_[pos.row] == 0) {
        row_cells_count_.erase(pos.row);
    }
    if(--col_cells_count_[pos.col] == 0) {
        col_cells_count_.erase(pos.col);
    }
}

Size Sheet::GetPrintableSize() const {
    return row_cells_count_.empty()
    ? Size{0, 0}
    : Size{row_cells_count_.rbegin()->first + 1,
           col_cells_count_.rbegin()->first + 1};
}


void Sheet::PrintCells(std::ostream& output, const std::function<void(const CellInterface&)>& printCell) const
{
    auto print_size = GetPrintableSize();
    for(int i = 0; i < print_size.rows; ++i) {
        bool add_delim = false;
        for (int j = 0; j < print_size.cols; ++j) {
            if(add_delim) {
                output << COL_DELIMITER;
            }
            add_delim = true;

            if(!CheckSize({i, j})) {
                continue;
            }
            auto& cell = cells_[i][j];
            if(!cell) {
                continue;
            }
            printCell(*cell);
        }
        output << ROW_DELIMITER;
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintCells(output, [&output](const CellInterface& cell) {
        std::visit([&output](const auto& val){output << val;}, cell.GetValue());
    });
}

void Sheet::PrintTexts(std::ostream& output) const {
    PrintCells(output, [&output]( const CellInterface& cell) {
        output << cell.GetText();
    });
}

void Sheet::MaybeIncreaseSizeToIncludePosition(Position pos) {
    if(pos.row >= static_cast<int>(cells_.size())) {
        cells_.resize(pos.row + 1);
    }
    if(pos.col >= static_cast<int>(cells_[pos.row].size())) {
        cells_[pos.row].resize(pos.col + 1);
    }
}

void Sheet::CheckPosition(Position pos) {
    if(!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if(!CheckSize(pos)) {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if(!CheckSize(pos)) {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

Cell& Sheet::GetOrCreateConcreteCell(Position pos) {
    MaybeIncreaseSizeToIncludePosition(pos);
    auto& cell = cells_[pos.row][pos.col];
    if(!cell) {
        cell = std::make_unique<Cell>(*this);
    }
    return *cell;
}

void Sheet::RemoveParent(Position pos, const Cell* old_cell) {
    auto& child_cell = cells_[pos.row][pos.col];
    child_cell->RemoveParent(old_cell);
    if(child_cell->IsEmpty() && child_cell->GetParents().empty()) {
        child_cell.reset();
    }
}


bool Sheet::CheckSize(Position pos) const {
    return static_cast<int>(cells_.size()) > pos.row
        && static_cast<int>(cells_[pos.row].size()) > pos.col;
}

bool Sheet::FindCyclicDeps(Position cell_pos, const Cell &cell) {
    if(cell.GetChildren().empty()) {
        return false;
    }
    std::unordered_set<const Cell*> visited;
    for(auto pos : cell.GetChildren()) {
        if(FindCyclicDepsImpl(visited, pos, cell_pos)) {
            return true;
        }
    }
    return false;
}

bool Sheet::FindCyclicDepsImpl(std::unordered_set<const Cell *> visited, Position pos, Position bad_pos) {
    if(pos == bad_pos) {
        return true;
    }
    const auto* cell = GetConcreteCell(pos);
    if(!cell || visited.count(cell)) {
        return false;
    }
    visited.emplace(cell);
    for(auto child_pos : cell->GetChildren()) {
        if(FindCyclicDepsImpl(visited, child_pos, bad_pos)) {
            return true;
        }
    }
    return false;
}

void Sheet::InvalidateCache(const Cell &cell) {
    cell.InvalidateCache();

    std::unordered_set<const Cell*> visited;
    for(const auto* parent_cell: cell.GetParents()) {
        InvalidateCacheImpl(*parent_cell, visited);
    }
}

void Sheet::InvalidateCacheImpl(const Cell &cell, std::unordered_set<const Cell *> visited) {
    if(visited.count(&cell)) {
        return;
    }
    cell.InvalidateCache();
    visited.emplace(&cell);
    for(const auto* parent_cell: cell.GetParents()) {
        InvalidateCacheImpl(*parent_cell, visited);
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}