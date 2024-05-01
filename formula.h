#pragma once

#include "common.h"

#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <unordered_set>
#include <functional>

class FormulaInterface {
  public:
  using Value = std::variant<double, FormulaError>;
  virtual ~FormulaInterface() = default;
  virtual Value Evaluate(const SheetInterface& sheet) const = 0;
  virtual std::string GetExpression() const = 0;
  virtual std::vector<Position> GetReferencedCells() const = 0;
};

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression);
