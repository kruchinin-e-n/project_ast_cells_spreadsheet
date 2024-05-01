#include "common.h"

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = { -1, -1 };

bool Position::operator==(const Position rhs) const {
  return std::tie(row, col) == std::tie(rhs.row, rhs.col);
}

bool Position::operator<(const Position rhs) const {
  return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
  return row >= 0 && row < MAX_ROWS && col >= 0 && col < MAX_COLS;
}

std::string Position::ToString() const {
  if (!IsValid()) { return EMPTY_SIGN; }
  int dec_to_26_digit_diff = 65;
  std::string index;
  index.reserve(MAX_POSITION_LENGTH);
  int column_int = col;
  while (column_int >= 0) {
    char to_insert = dec_to_26_digit_diff + (column_int % LETTERS);
    index.insert(index.begin(), to_insert);
    column_int /= LETTERS;
    --column_int;
  }

  return index += std::to_string(row + 1);
}

Position Position::FromString(std::string_view str) {
  // Find the first digit character in the string
  auto it = std::find_if(
      str.begin(),
      str.end(),
      [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
      });

  // If no digit is found or it's at the beginning, return Position::NONE
  if (it == str.begin()) { return Position::NONE; }
  //Split the string into literals...
  auto literals = str.substr(0, it - str.begin());
  // ...and digitals
  auto digitals = str.substr(it - str.begin());

  // Check if literals or digitals are empty
  if (literals.empty() || digitals.empty()) { return Position::NONE; }
  // Check if the number of letters exceeds the maximum allowed
  if (literals.size() > MAX_POS_LETTER_COUNT) { return Position::NONE; };

  // Check if all characters in literals are uppercase letters
  if (std::any_of(literals.begin(),
                  literals.end(),
                  [](char ch) {
                    return !std::isalpha(ch) || !std::isupper(ch);
                  })) {
    return Position::NONE;
  }
  // Check if all characters in digitals are digits
  if (std::any_of(digitals.begin(),
                  digitals.end(),
                  [](char ch) {
                    return !std::isdigit(static_cast<unsigned char>(ch)); })) {
    return Position::NONE;
  }

  // Parse the row from digitals
  int row;

  try { row = std::stoi(std::string(digitals)); }
  catch (const std::out_of_range&) { return Position::NONE; }

  // Parse the column from literals
  int col = 0;
  int ascii_to_26_digit_diff = 64;

  for (char ch : literals) {
    col *= LETTERS;
    col += ch - ascii_to_26_digit_diff;
  }

  // Return the parsed position
  return { row - 1, col - 1 };
}

bool Size::operator==(Size rhs) const {
  return rows == rhs.rows && cols == rhs.cols;
}
