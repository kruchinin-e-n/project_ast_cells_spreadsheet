#pragma once

#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <cctype>
#include <sstream>
#include <tuple>
#include <algorithm>

// Cell position. Zero-based indexing.
struct Position {
  int row = 0;
  int col = 0;
  static const int MAX_ROWS = 16384;
  static const int MAX_COLS = 16384;
  static const Position NONE;

  bool operator==(Position rhs) const;
  bool operator<(Position rhs) const;
  bool IsValid() const;
  std::string ToString() const;
  static Position FromString(std::string_view str);
};

struct PositionHasher {
  // Hash function for Position objects, combine hash values of row
  // and col using addition and multiplication
  std::size_t operator()(const Position& pos) const {
    // Choose a prime number as initial value
    std::size_t hash_value = 17;
    hash_value = hash_value * 31 + std::hash<int>()(pos.row);
    hash_value = hash_value * 31 + std::hash<int>()(pos.col);
    return hash_value;
  }
};

struct Size {
  int rows = 0;
  int cols = 0;

  bool operator==(Size rhs) const;
};

// Describes the errors that can occur when calculating a formula.
class FormulaError {
  public:

  enum class Category { Ref, Value, Div0,};
  FormulaError(Category category);
  Category GetCategory() const;
  bool operator==(FormulaError rhs) const;
  std::string_view Message() const;

  private:
  Category category_;
};

std::ostream& operator<<(std::ostream& output, FormulaError fe);

// Exception thrown when attempting to pass an invalid position to a method.
class InvalidPositionException : public std::out_of_range {
  public:
  using std::out_of_range::out_of_range;
};

// Exception thrown when attempting to set a syntactically incorrect formula
class FormulaException : public std::runtime_error {
  public:
  using std::runtime_error::runtime_error;
};

class CircularDependencyException : public std::runtime_error {
  public:
  using std::runtime_error::runtime_error;
};

class CellInterface {
  public:
  // Either the cell's text, the formula's value,
  // or an error message from the formula
  using Value = std::variant<std::string, double, FormulaError>;

  virtual ~CellInterface() = default;
  // Returns the visible value of the cell.
  // In the case of a text cell, it's the text (without escape characters). In
  // the case of a formula, it's the numerical
  // value of the formula or an error message.
  virtual Value GetValue() const = 0;
  // Returns the internal text of the cell, as if we were editing it.
  // In the case of a text cell, it's its text
  // (potentially containing escape characters).
  // In the case of a formula - it's its expression.
  virtual std::string GetText() const = 0;
  virtual std::vector<Position> GetReferencedCells() const = 0;
};

inline constexpr char FORMULA_SIGN = '=';
inline constexpr char ESCAPE_SIGN = '\'';
inline std::string EMPTY_SIGN = "";

// Table interface
class SheetInterface {
  public:
  virtual ~SheetInterface() = default;
  // Sets the content of the cell.
  // * If the text begins with the character "'" (apostrophe),
  // it is omitted when the cell's value is returned by the GetValue() method.
  // It can be used if it's necessary to start the text with the sign "=",
  // but without having it interpreted as a formula.
  virtual void SetCell(Position pos, std::string text) = 0;
  // Returns the cell value.
  // If the cell is empty, it might return nullptr.
  virtual const CellInterface* GetCell(Position pos) const = 0;
  virtual CellInterface* GetCell(Position pos) = 0;
  // Clears the cell.
  // Subsequent calls to GetCell() for this cell will return either nullptr, or
  // an object with an empty text.
  virtual void ClearCell(Position pos) = 0;
  // Calculates the size of the area involved in printing.
  // This is determined as the bounding rectangle of all cells with non-empty
  // text.
  virtual Size GetPrintableSize() const = 0;
  // Outputs the entire table to the passed stream. Columns are separated by a
  // tab character. A line break character is output after each row.
  // The GetValue() or GetText() methods are used to convert cells to a string,
  // respectively. An empty cell is represented by an empty string in any case.
  virtual void PrintValues(std::ostream& output) const = 0;
  virtual void PrintTexts(std::ostream& output) const = 0;
};

// Creates an empty table ready for use.
std::unique_ptr<SheetInterface> CreateSheet();
