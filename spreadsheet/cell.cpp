#include "cell.h"
#include "formula.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::Impl
{
public:
    virtual Value GetValue(const SheetInterface& sheet) = 0;
    virtual std::string GetText() = 0;
    virtual std::vector<Position> GetReferencedCells() = 0;

    virtual ~Impl() = default;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    EmptyImpl() = default;

    Value GetValue(const SheetInterface& sheet) override {
        return "";
    }

    std::string GetText() override {
        return "";
    }

    std::vector<Position> GetReferencedCells() override {
        return {};
    }
};

class Cell::TextImpl : public Cell::Impl {
public:
    TextImpl(std::string text) : text_(std::move(text)) {}

    Value GetValue(const SheetInterface& sheet) override {
        return text_.front() == ESCAPE_SIGN
        ? text_.substr(1)
        : text_;
    }

    std::string GetText() override {
        return text_;
    }

    std::vector<Position> GetReferencedCells() override {
        return {};
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    FormulaImpl(const std::string& text) : formula_(ParseFormula(text.substr(1))) {}

    Value GetValue(const SheetInterface& sheet) override {
        return std::visit([](auto&& arg) -> Value {return arg;}, formula_->Evaluate(sheet));
    }

    std::string GetText() override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() override {
        return formula_->GetReferencedCells();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
};

Cell::Cell(const SheetInterface &sheet, std::string text) : sheet_(sheet) {
    if(text.front() == FORMULA_SIGN && text.size() > 1) {
        impl_ = std::make_unique<FormulaImpl>(std::move(text));
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
    children_ = impl_->GetReferencedCells();
}

Cell::Cell(const SheetInterface &sheet) : sheet_(sheet) {
    impl_ = std::make_unique<EmptyImpl>();
}

void Cell::AddParent(const Cell *parent) {
    parents_.emplace(parent);
}

void Cell::RemoveParent(const Cell *parent) {
    parents_.erase(parent);
}

void Cell::SetParents(Cell::Parents parents) {
    parents_ = std::move(parents);
}

Cell::Parents& Cell::GetParents() {
    return parents_;
}

const Cell::Parents& Cell::GetParents() const {
    return parents_;
}

const Cell::Children &Cell::GetChildren() const {
    return children_;
}

std::vector<Position> Cell::GetReferencedCells() const {
    return children_;
}

void Cell::InvalidateCache() const {
    cache_.reset();
}

bool Cell::IsEmpty() const {
    return dynamic_cast<EmptyImpl*>(impl_.get());
}

Cell::~Cell() = default;

Cell::Value Cell::GetValue() const {
    if(!cache_) {
        cache_ = impl_->GetValue(sheet_);
    }
    return *cache_;
}
std::string Cell::GetText() const {
    return impl_->GetText();
}