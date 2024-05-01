#include "sheet.h"
#include "cell.h"
#include "common.h"

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
  if (!pos.IsValid()) {
    throw InvalidPositionException("Error: position is not valid");
  }

  const auto& cell = sheet_.find(pos);

  if (cell == sheet_.end()) {
    sheet_.emplace(pos, std::make_unique<Cell>(*this));
  }
  sheet_.at(pos)->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
  return GetConcreteCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) { return GetConcreteCell(pos); }

void Sheet::ClearCell(Position pos) {
  // Check if the position is valid
  if (!pos.IsValid()) {
    throw InvalidPositionException("Error: position is not valid");
  }

  // Find the cell at the given position
  const auto& cell_at_pos = sheet_.find(pos);
  if (cell_at_pos != sheet_.end() && cell_at_pos->second != nullptr) {
    // Clear the cell's content
    cell_at_pos->second->Clear();
    // Check if the cell is no longer referenced and reset it if necessary
    if (!cell_at_pos->second->IsReferenced()) { cell_at_pos->second.reset(); }
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
  // Get the printable size of the sheet
  Size size = GetPrintableSize();
  // Iterate through each cell and print its value
  for (int row = 0; row < size.rows; ++row) {
    for (int col = 0; col < size.cols; ++col) {
      // Print tab separator between columns (except for the first column)
      if (col > 0) { output << "\t"; }
      // Find the cell at the current position
      const auto& it = sheet_.find({ row, col });
      // Check if the cell exists and has a non-empty text value
      if (it != sheet_.end() && it->second != nullptr
          && !it->second->GetText().empty()) {
        const auto& value = it->second->GetValue();
        // Check the type of the value and print it to the output stream
        if (std::holds_alternative<double>(value)) {
          output << std::get<double>(value);
        }
        // Check the type of the value and print it to the output stream
        else if (std::holds_alternative<std::string>(value)) {
          output << std::get<std::string>(value);
        }
        // Check the type of the value and print it to the output stream
        else if (std::holds_alternative<FormulaError>(value)) {
          output << std::get<FormulaError>(value).Message();
        }
      }
    }
    // Print newline character to move to the next row
    output << "\n";
  }
}

void Sheet::PrintTexts(std::ostream& output) const {
  // Get the printable size of the sheet
  Size size = GetPrintableSize();
  // Iterate through each cell and print its text
  for (int row = 0; row < size.rows; ++row) {
    for (int col = 0; col < size.cols; ++col) {
      // Print tab separator between columns (except for the first column)
      if (col > 0) output << "\t";
      // Find the cell at the current position
      const auto& it = sheet_.find({ row, col });
      // Check if the cell exists and has non-empty text
      if (it != sheet_.end() && it->second != nullptr
          && !it->second->GetText().empty()) {
        // Print the text of the cell
        output << it->second->GetText();
      }
    }
    // Print newline character to move to the next row
    output << "\n";
  }
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
  // Check if the position is valid
  if (!pos.IsValid()) {
    throw InvalidPositionException("Error: position is not valid");
  }
  // Find the cell at the specified position
  const auto requested_cell_iter = sheet_.find(pos);
  // If the cell does not exist, return nullptr
  if (requested_cell_iter == sheet_.end()) { return nullptr; }
  // Return a pointer to the cell at the specified position
  return requested_cell_iter->second.get();
}

Cell* Sheet::GetConcreteCell(Position pos) {
  return const_cast<Cell*>(
      static_cast<const Sheet&>(*this).GetConcreteCell(pos));
}

std::unique_ptr<SheetInterface> CreateSheet() {
  return std::make_unique<Sheet>();
}
