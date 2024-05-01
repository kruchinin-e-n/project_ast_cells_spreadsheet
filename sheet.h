#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

class Sheet : public SheetInterface {
  public:

  ~Sheet();

  void SetCell(Position pos, std::string text) override;

  const CellInterface* GetCell(Position pos) const override;

  CellInterface* GetCell(Position pos) override;

  void ClearCell(Position pos) override;

  Size GetPrintableSize() const override;

  void PrintValues(std::ostream& output) const override;

  void PrintTexts(std::ostream& output) const override;

  const Cell* GetConcreteCell(Position pos) const;

  Cell* GetConcreteCell(Position pos);

  private:

  class SheetHasher {
    public:
    size_t operator()(const Position pos) const {
      return std::hash<std::string>()(pos.ToString());
    }
  };

  class SheetKeyEqual {
    public:
    bool operator()(const Position& lhs, const Position& rhs) const {
      return lhs == rhs;
    }
  };

  std::unordered_map<Position, std::unique_ptr<Cell>,
                     SheetHasher,
                     SheetKeyEqual> sheet_;
};

std::unique_ptr<SheetInterface> CreateSheet();
