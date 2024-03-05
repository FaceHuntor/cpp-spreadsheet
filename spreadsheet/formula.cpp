#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;


namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression);
    Value Evaluate(const SheetInterface& sheet) const override;
    std::string GetExpression() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}

Formula::Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression)) {

} catch(const std::exception& ex) {
    throw FormulaException(ex.what());
}

class ValueToDouble {
public:
    double operator()(double val) const {
        return val;
    }

    double operator()(const std::string val) const {
        if(val.empty()) {
            return 0;
        }
        char* endptr = nullptr;
        double res = strtod(val.data(), &endptr);
        if(*endptr != '\0' || endptr == val.data()) {
            throw FormulaError(FormulaError::Category::Value);
        }
        return res;
    }

    double operator()(const FormulaError error) const {
        throw error;
    }
};

FormulaInterface::Value Formula::Evaluate(const SheetInterface& sheet) const {
    try {
        return ast_.Execute([&sheet](Position pos) -> double {
            const CellInterface *cell;
            try {
                cell = sheet.GetCell(pos);
            } catch (const InvalidPositionException&) {
                throw FormulaError(FormulaError::Category::Ref);
            }
            if(!cell) {
                return 0;
            }

            auto val = cell->GetValue();
            return std::visit(ValueToDouble(), val);
        });
    }
    catch (const FormulaError& ex) {
        return ex;
    }
}

std::string Formula::GetExpression() const {
    std::ostringstream ss;
    ast_.PrintFormula(ss);
    return ss.str();
}

std::vector<Position> Formula::GetReferencedCells() const {
    std::set<Position> cells = {ast_.GetCells().begin(), ast_.GetCells().end()};
    return {cells.begin(), cells.end()};
}