#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Sheet;

class Cell : public CellInterface {
    private:
        class Impl;

        class EmptyImpl;

        class TextImpl;

        class FormulaImpl;

        bool CheckForCircularDependencies(const Impl& new_impl) const;

    public:
        Cell(Sheet& sheet);

        ~Cell();

        void Set(std::string text);

        void Clear();

        Value GetValue() const override;

        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        void InvalidateIncomingCellsCache();

        bool IsReferenced() const;

    private:
        std::unique_ptr<Impl> impl_;

        std::unordered_set<Cell*> incoming_cells_;

        std::unordered_set<Cell*> outgoing_cells_;

        Sheet& sheet_;
};