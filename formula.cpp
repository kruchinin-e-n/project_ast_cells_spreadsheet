#include "formula.h"
#include "FormulaAST.h"

using namespace std::literals;

FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const { return category_; }

bool FormulaError::operator==(FormulaError rhs) const { return category_ == rhs.category_; }

std::string_view FormulaError::Message() const {
	switch (category_) {
		case Category::Ref:   return "#REF!";
		case Category::Value: return "#VALUE!";
		case Category::Div0:  return "#ARITHM!";
	}
	return EMPTY_SIGN;
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) { return output << fe.Message(); }

namespace {
	class Formula : public FormulaInterface {
	public:
		explicit Formula(std::string expression)
		: ast_(ParseFormulaAST(expression)) {}

		Value Evaluate(const SheetInterface& sheet) const override {
			const std::function<double(Position)> args = [&sheet](const Position pos)->double {		// Define a lambda function to handle arguments in the formula
				if (!pos.IsValid()) { throw FormulaError(FormulaError::Category::Ref); }		// If the position is not valid, throw a reference error

				const auto* cell = sheet.GetCell(pos);		// Get the cell at the given position
				if (!cell) return 0.0;		// If the cell doesn't exist, return 0.0
				const auto& value = cell->GetValue();		 // Retrieve the value of the cell
				if (std::holds_alternative<double>(value)) { return std::get<double>(value); }		// Check the type of the value and convert it to double if possible
				else if (std::holds_alternative<std::string>(value)) {		// If the value is a string, attempt to convert it to double
					auto string_value = std::get<std::string>(value);
					double result = 0.0;
					if (!string_value.empty()) {
						std::istringstream in(string_value);
						if (!(in >> result) || !in.eof()) {		 // Try to extract a double value from the string
							throw FormulaError(FormulaError::Category::Value);		 // If extraction fails or not all characters are consumed, throw a value error
						}
					}
					return result;
				}
				else if (std::holds_alternative<FormulaError>(value)) { throw std::get<FormulaError>(value); }		// If the value is a FormulaError, throw it

				throw FormulaError(FormulaError::Category::Value);		// Unknown value type encountered
			};

			try { return ast_.Execute(args); }	// Execute AST using the provided arguments
			catch (FormulaError& error) { return error; }		// Catch any FormulaError thrown during execution and return it
		}

		std::vector<Position> GetReferencedCells() const override {
			std::vector<Position> referenced_cells;		// Vector to store unique referenced cells
			std::unordered_set<Position, PositionHasher> unique_cells;		// Set to store unique cell positions
			for (const auto& referenced_cell : ast_.GetCells()) {		// Iterate through the cells in AST
				if (referenced_cell.IsValid() && unique_cells.insert(referenced_cell).second) {		// Check if the cell position is valid and not already present in the set
					referenced_cells.push_back(referenced_cell);		// Add valid cell positions to the vector
				}
			}
			return referenced_cells;		// Return the vector of unique referenced cells
		}

		std::string GetExpression() const override {
			std::ostringstream output;		// Create an output string stream to store the expression
			ast_.PrintFormula(output);		// Print the formula expression to the output stream
			return output.str();			// Convert the content of the output stream to a string and return it
		}

	private:
		const FormulaAST ast_;
	};
}

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
	try { return std::make_unique<Formula>(std::move(expression)); }		// Try to create a unique pointer to a Formula object using the provided expression
	catch (...) { throw FormulaException(EMPTY_SIGN); }		// If an exception is caught during formula parsing, throw a FormulaException
}