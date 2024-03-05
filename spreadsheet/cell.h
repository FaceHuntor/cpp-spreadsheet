#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;

class Cell : public CellInterface {
public:
    using Children = std::vector<Position>;
    using Parents = std::unordered_set<const Cell*>;

    Cell(const SheetInterface &sheet, std::string text);
    Cell(const SheetInterface &sheet);
    ~Cell() override;

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    void AddParent(const Cell* parent);
    void RemoveParent(const Cell* parent);
    void SetParents(Parents parents);
    const Parents& GetParents() const;
    Parents& GetParents();
    const Children& GetChildren() const;


    void InvalidateCache() const;
    bool IsEmpty() const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    const SheetInterface& sheet_;
    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cache_;
    Children children_;  //  cells this cell depends on
    Parents parents_;  //  cells that depend on the current cell
};