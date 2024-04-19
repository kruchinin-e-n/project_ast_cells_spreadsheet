#include "sheet.h"
#include "cell.h"
#include "common.h"

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
	if (!pos.IsValid()) throw InvalidPositionException("Error: position is not valid");

	const auto& cell = sheet_.find(pos);

	if (cell == sheet_.end()) sheet_.emplace(pos, std::make_unique<Cell>(*this));
	sheet_.at(pos)->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const { return GetConcreteCell(pos); }

CellInterface* Sheet::GetCell(Position pos) { return GetConcreteCell(pos); }

void Sheet::ClearCell(Position pos) {
	if (!pos.IsValid()) {		// Check if the position is valid
		throw InvalidPositionException("Error: position is not valid");
	}

	const auto& cell_at_pos = sheet_.find(pos);		// Find the cell at the given position
	if (cell_at_pos != sheet_.end() && cell_at_pos->second != nullptr) {
		cell_at_pos->second->Clear();		// Clear the cell's content
		if (!cell_at_pos->second->IsReferenced()) { cell_at_pos->second.reset(); }		// Check if the cell is no longer referenced and reset it if necessary
	}
}

Size Sheet::GetPrintableSize() const {

	int max_row = 0;
	int max_col = 0;

	for (auto it = sheet_.begin(); it != sheet_.end(); ++it) {
		if (it->second != nullptr) {
			if (it->first.row > max_row - 1) { max_row = it->first.row + 1; }
			if (it->first.col > max_col - 1) { max_col = it->first.col + 1; }
		}
	}

	return Size{ max_row, max_col };
}

void Sheet::PrintValues(std::ostream& output) const {
	Size size = GetPrintableSize();		// Get the printable size of the sheet
	for (int row = 0; row < size.rows; ++row) {		// Iterate through each cell and print its value
		for (int col = 0; col < size.cols; ++col) {
			if (col > 0) { output << "\t"; }		// Print tab separator between columns (except for the first column)
			const auto& it = sheet_.find({ row, col });		// Find the cell at the current position
			if (it != sheet_.end() && it->second != nullptr && !it->second->GetText().empty()) {		// Check if the cell exists and has a non-empty text value
				const auto& value = it->second->GetValue();
				if (std::holds_alternative<double>(value)) {		// Check the type of the value and print it to the output stream
					output << std::get<double>(value);
				}
				else if (std::holds_alternative<std::string>(value)) {		// Check the type of the value and print it to the output stream
					output << std::get<std::string>(value);
				}
				else if (std::holds_alternative<FormulaError>(value)) {		// Check the type of the value and print it to the output stream
					output << std::get<FormulaError>(value).Message();
				}
			}
		}
		output << "\n";		 // Print newline character to move to the next row
	}
}

void Sheet::PrintTexts(std::ostream& output) const {
	Size size = GetPrintableSize();		// Get the printable size of the sheet
	for (int row = 0; row < size.rows; ++row) {		// Iterate through each cell and print its text
		for (int col = 0; col < size.cols; ++col) {
			if (col > 0) output << "\t";		// Print tab separator between columns (except for the first column)
			const auto& it = sheet_.find({ row, col });		// Find the cell at the current position
			if (it != sheet_.end() && it->second != nullptr && !it->second->GetText().empty()) {		// Check if the cell exists and has non-empty text
				output << it->second->GetText();		 // Print the text of the cell
			}
		}
		output << "\n";		 // Print newline character to move to the next row
	}
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
	if (!pos.IsValid()) throw InvalidPositionException("Error: position is not valid");		// Check if the position is valid
	const auto requested_cell_iter = sheet_.find(pos);		// Find the cell at the specified position
	if (requested_cell_iter == sheet_.end()) { return nullptr; }		// If the cell does not exist, return nullptr
	return requested_cell_iter->second.get();		// Return a pointer to the cell at the specified position
}

Cell* Sheet::GetConcreteCell(Position pos) { return const_cast<Cell*>(static_cast<const Sheet&>(*this).GetConcreteCell(pos)); }

std::unique_ptr<SheetInterface> CreateSheet() { return std::make_unique<Sheet>(); }