#include "cell.h"
#include "sheet.h"

class Cell::Impl {
	public:
		virtual ~Impl() = default;
		virtual Value GetValue() const = 0;
		virtual std::string GetText() const = 0;
		virtual std::vector<Position> GetReferencedCells() const { return {}; }
		virtual bool IsCacheValid() const { return true; }
		virtual void InvalidateOneCellCache() {}
	};

class Cell::EmptyImpl : public Impl {
	public:
		Value GetValue() const override { return EMPTY_SIGN; }
		std::string GetText() const override { return EMPTY_SIGN; }
};

class Cell::TextImpl : public Impl {
	public:
		TextImpl(std::string text) : text_(std::move(text)) {}

		Value GetValue() const override { return text_[0] == ESCAPE_SIGN ? text_.substr(1) : text_; }

		std::string GetText() const override { return text_; }

	private:
		std::string text_;
};

class Cell::FormulaImpl : public Impl {
	public:
		explicit FormulaImpl(std::string expression, const SheetInterface& sheet) : sheet_(sheet) {
			expression.empty() || expression[0] != FORMULA_SIGN ? throw std::logic_error(EMPTY_SIGN) : formula_ptr_ = ParseFormula(expression.substr(1));
		}

		Value GetValue() const override {
			if (!cache_) { cache_ = formula_ptr_->Evaluate(sheet_); }		// If the value is not cached, evaluate the formula and cache the result
			auto value = formula_ptr_->Evaluate(sheet_);		// Retrieve the value from the evaluated formula
			if (std::holds_alternative<double>(value)) return std::get<double>(value);	// Check the type of the value and return accordingly, if the value is a double, return it
			return std::get<FormulaError>(value);		// If the value is a FormulaError, return it
		}

		std::string GetText() const override { return FORMULA_SIGN + formula_ptr_->GetExpression(); }

		bool IsCacheValid() const override { return cache_.has_value(); }

		void InvalidateOneCellCache() override { cache_.reset(); }

		std::vector<Position> GetReferencedCells() const { return formula_ptr_->GetReferencedCells(); }

	private:
		std::unique_ptr<FormulaInterface> formula_ptr_;
		const SheetInterface& sheet_;
		mutable std::optional<FormulaInterface::Value> cache_;
};

bool Cell::CheckForCircularDependencies(const Impl& impl_being_checked) const {
	if (impl_being_checked.GetReferencedCells().empty()) { return false; }		// If there are no references to other cells, there are no circular dependencies

	std::unordered_set<const Cell*> referenced_cells;		// Create a set of cells referenced by the current cell
	for (const auto& pos : impl_being_checked.GetReferencedCells()) {
		referenced_cells.insert(sheet_.GetConcreteCell(pos));
	}

	std::unordered_set<const Cell*> checked_cells;		// Set for checked cells
	std::unordered_set<const Cell*> unchecked_cells;		// Set for checked and unchecked cells

	unchecked_cells.insert(this);		// Add the current cell to unchecked cells

	while (!unchecked_cells.empty()) {		// While there are unchecked cells
		const Cell* cell_being_checked = *unchecked_cells.begin();		// Take a cell to be checked
		unchecked_cells.erase(cell_being_checked);
		checked_cells.insert(cell_being_checked);

		if (referenced_cells.count(cell_being_checked)) { return true; }		// If the current cell is a reference to a cell referenced by the checked cell, there is a circular dependency

		for (const Cell* incoming_cell : cell_being_checked->incoming_cells_) {		// Check all incoming cells for the current one
			if (!checked_cells.count(incoming_cell)) {		// If the cell hasn't been checked yet, add it to the list for checking
				unchecked_cells.insert(incoming_cell);
			}
		}
	}
	return false;		// In case no circular dependencies were found
}

Cell::Cell(Sheet& sheet) : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

Cell::~Cell() {}

void Cell::Set(std::string text) {
	std::unique_ptr<Impl> temporary_impl;		// Create a temporary implementation pointer

	// Determine the type of implementation based on the input text
	if (text.empty()) temporary_impl = std::make_unique<EmptyImpl>();		// If text is empty, use EmptyImpl
	else if (text.size() > 1 && text[0] == FORMULA_SIGN) temporary_impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);		// If text starts with the formula sign, use FormulaImpl
	else temporary_impl = std::make_unique<TextImpl>(std::move(text));		// Otherwise, use TextImpl
	if (CheckForCircularDependencies(*temporary_impl)) { throw CircularDependencyException(EMPTY_SIGN); }		// Check for circular dependencies before applying changes
	for (Cell* outgoing : outgoing_cells_) { outgoing->incoming_cells_.erase(this); }		// Remove this cell from the outgoing cells of its outgoing cells
	outgoing_cells_.clear();

	for (const auto& pos : impl_->GetReferencedCells()) {		// Update outgoing cells and incoming references based on the new implementation
		Cell* outgoing = sheet_.GetConcreteCell(pos);
		if (!outgoing) {
			sheet_.SetCell(pos, EMPTY_SIGN);
			outgoing = sheet_.GetConcreteCell(pos);
		}
		outgoing_cells_.insert(outgoing);
		outgoing->incoming_cells_.insert(this);
	}

	impl_ = std::move(temporary_impl);		// Replace the current implementation with the new one

	InvalidateIncomingCellsCache();		// Invalidate the cache of incoming cells
}

void Cell::Clear() { impl_ = std::make_unique<EmptyImpl>(); }

Cell::Value Cell::GetValue() const { return impl_->GetValue(); }

std::string Cell::GetText() const { return impl_->GetText(); }

std::vector<Position> Cell::GetReferencedCells() const { return impl_->GetReferencedCells(); }

void Cell::InvalidateIncomingCellsCache() {
	impl_->InvalidateOneCellCache();
	for (Cell* incoming_cell : incoming_cells_) { incoming_cell->InvalidateIncomingCellsCache(); }
}

bool Cell::IsReferenced() const { return !incoming_cells_.empty(); }