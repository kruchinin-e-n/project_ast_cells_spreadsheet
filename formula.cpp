#include "formula.h"
#include "FormulaAST.h"

using namespace std::literals;

FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const { return category_; }

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::Message() const {
  switch (category_) {
    case Category::Ref:   return "#REF!";
    case Category::Value: return "#VALUE!";
    case Category::Div0:  return "#ARITHM!";
  }
  return EMPTY_SIGN;
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.Message();
}

namespace {

class Formula : public FormulaInterface {
  public:
  explicit Formula(std::string expression)
    : ast_(ParseFormulaAST(expression)) {
  }

  Value Evaluate(const SheetInterface& sheet) const override {
    // Define a lambda function to handle arguments in the formula
    const std::function<double(Position)> args =
        [&sheet](const Position pos)->double {
      // If the position is not valid, throw a reference error
      if (!pos.IsValid()) { throw FormulaError(FormulaError::Category::Ref); }

      // Get the cell at the given position
      const auto* cell = sheet.GetCell(pos);
      // If the cell doesn't exist, return 0.0
      if (!cell) { return 0.0; }
      // Retrieve the value of the cell
      const auto& value = cell->GetValue();
      // Check the type of the value and convert it to double if possible
      if (std::holds_alternative<double>(value)) {
          return std::get<double>(value);
      }
      // If the value is a string, attempt to convert it to double
      else if (std::holds_alternative<std::string>(value)) {
        auto string_value = std::get<std::string>(value);
        double result = 0.0;
        if (!string_value.empty()) {
          std::istringstream in(string_value);
          // Try to extract a double value from the string
          if (!(in >> result) || !in.eof()) {
            // If extraction fails or not all characters
            // are consumed, throw a value error
            throw FormulaError(FormulaError::Category::Value);
          }
        }
        return result;
      }
      // If the value is a FormulaError, throw it
      else if (std::holds_alternative<FormulaError>(value)) {
        throw std::get<FormulaError>(value);
      }
      // Unknown value type encountered
      throw FormulaError(FormulaError::Category::Value);
    };

    // Execute AST using the provided arguments
    try { return ast_.Execute(args); }
    // Catch any FormulaError thrown during execution and return it
    catch (FormulaError& error) { return error; }
  }

  std::vector<Position> GetReferencedCells() const override {
    // Vector to store unique referenced cells
    std::vector<Position> referenced_cells;
    // Set to store unique cell positions
    std::unordered_set<Position, PositionHasher> unique_cells;

    // Iterate through the cells in AST
    for (const auto& referenced_cell : ast_.GetCells()) {
      // Check if the cell position is valid and not already present in the set
      if (referenced_cell.IsValid()
          && unique_cells.insert(referenced_cell).second) {
        // Add valid cell positions to the vector
        referenced_cells.push_back(referenced_cell);
      }
    }
    // Return the vector of unique referenced cells
    return referenced_cells;
  }

  std::string GetExpression() const override {
    // Create an output string stream to store the expression
    std::ostringstream output;
    // Print the formula expression to the output stream
    ast_.PrintFormula(output);
    // Convert the content of the output stream to a string and return it
    return output.str();
  }

  private:

  const FormulaAST ast_;
};

} // end of namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
  // Try to create a unique pointer to a Formula
  // object using the provided expression
  try { return std::make_unique<Formula>(std::move(expression)); }
  // If an exception is caught during formula parsing, throw a FormulaException
  catch (...) { throw FormulaException(EMPTY_SIGN); }
}
