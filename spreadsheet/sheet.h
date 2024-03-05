#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>
#include <vector>
#include <unordered_set>

class Sheet : public SheetInterface {
public:
    ~Sheet() override;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;



private:
    void MaybeIncreaseSizeToIncludePosition(Position pos);
    void PrintCells(std::ostream& output,
                    const std::function<void(const CellInterface&)>& printCell) const;
    static void CheckPosition(Position pos);

    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);
    Cell& GetOrCreateConcreteCell(Position pos);
    void RemoveParent(Position pos, const Cell* old_cell);

    bool FindCyclicDeps(Position cell_pos, const Cell& cell);
    bool FindCyclicDepsImpl(std::unordered_set<const Cell*> visited, Position pos, Position bad_pos);
    void InvalidateCache(const Cell& cell);
    void InvalidateCacheImpl(const Cell& cell, std::unordered_set<const Cell*> visited);

    bool CheckSize(Position pos) const;

//    template<typename Callable>
//    void PrintCells(std::ostream& output, const Callable& print_method) const;

	// Можете дополнить ваш класс нужными полями и методами

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;
    std::map<int, size_t> row_cells_count_;
    std::map<int, size_t> col_cells_count_;
};