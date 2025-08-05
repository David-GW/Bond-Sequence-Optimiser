#include "types/BondReturnData.h"

#include "helpers/GeneralHelpers.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <fstream>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "helpers/InputHelpers.h"

/// Constructor for BondReturnData.
BondReturnData::BondReturnData(std::vector<int> t, const std::size_t m, std::vector<double> g):
	tenors_(std::move(t)),
	numMonths_(m),
	grid_(std::move(g))
{
	if (numMonths_ == 0) {
		throw std::invalid_argument("BondReturnData: must have at least 1 month");
	}

	const std::size_t numTenors = tenors_.size();

	if (grid_.size() != numTenors * numMonths_) {
		throw std::invalid_argument("BondReturnData: size mismatch");
	}

	rowPointers_.reserve(numTenors);
	for (std::size_t currentRowNum = 0; currentRowNum < numTenors; ++currentRowNum) {
		rowPointers_.push_back(&grid_[currentRowNum * numMonths_]);
	}
}

/**
* Getter for bond return values of a BondReturnData grid in the natural way,
* taking the row (of the desired tenor in the sorted grid) and month as (row, month).
*
* While stored as a 1D-vector for speed, this allows access as if grid were a 2D-vector, using the row pointers for efficiency.
*/
double BondReturnData::operator()(const int row, const int month) const {
	if (row < 0 || row >= static_cast<int>(rowPointers_.size())) {
		throw std::invalid_argument("BondReturnData: row out of range");
	}
	if (month < 0 || month >= static_cast<int>(numMonths_)) {
		throw std::invalid_argument("BondReturnData: month out of range");
	}
	return rowPointers_[row][month];
}

/// Contains the machinery necessary to safely load the bond return data from the provided file.
namespace CSVLoader
{
	/// Contains helper functions used only in the CSVLoader namespace.
	namespace Detail
	{
		[[nodiscard]] static bool isBlankLineCSV(std::string_view sv) {
			return std::ranges::all_of(sv, [](const char c) {
				return c == ' ' || c == '\t' || c == '\n' || c == '\r';
			});
		}

		/// Checks if an extension matches that of common spreadsheet formats, since a user may mistake this for a CSV file.
		[[nodiscard]] static bool isSpreadsheetExtension(const std::string& ext) {
			static const std::unordered_set<std::string_view> spreadsheetExtensions = {
				"xlsx", "xls", "xlsm", "xlsb", "numbers", "ods"
			};
			return spreadsheetExtensions.contains(ext);
		}

		/// Takes a row of CSV data and returns string_views of the cells split at ','.
		[[nodiscard]] static auto getCells(std::string_view row) {
			return row | std::views::split(',') | std::views::transform([](auto&& v) {
				return std::string_view{ std::ranges::begin(v), std::ranges::end(v) };
			});
		}

		/// Takes a string_view storing a potential tenor and returns it as an integer after checking validity.
		[[nodiscard]] static int parseTenor(std::string_view sv) {
			// Remove whitespace from start and end.
			GeneralHelpers::svTrimWhitespace(sv);
			if (sv.empty()) {
				throw std::runtime_error("tenor is empty");
			}

			// Check that all remaining characters are digits.
			if (!GeneralHelpers::svAllDigits(sv)){
				throw std::runtime_error("tenor must be a positive integer");
			}

			int result;
			// Stores any error arising from trying to convert the string_view into an integer via std::from_chars.
			const std::errc e = std::from_chars(sv.data(), sv.data() + sv.size(), result).ec;

			if (e == std::errc::result_out_of_range) {
				throw std::runtime_error("tenor is too long");
			}
			if (e != std::errc()) {
				throw std::runtime_error("invalid tenor");
			}
			if (result == 0) {
				throw std::runtime_error("tenor must be a positive integer");
			}
			return result;
		}

		/// Takes a string_view storing a potential bond return and returns it as a double after checking validity.
		[[nodiscard]] static double parseBondReturn(std::string_view sv) {
			// Remove whitespace from start and end.
			GeneralHelpers::svTrimWhitespace(sv);

			if (sv.empty()) {
				throw std::runtime_error("bond return is empty");
			}

			// Check if signed.
			bool isNegative = false;
			if (sv.front() == '+' || sv.front() == '-') {
				isNegative = sv.front() == '-';
				sv.remove_prefix(1);  // remove the '-' for further digit checks
			}

			// Check that all characters are digits, or at most one '.'
			// (so we cannot simply call GeneralHelpers::svTrimWhitespace).
			bool periodFound = false;
			if (!std::ranges::all_of(sv, [&](const char c) {
				if (c == '.') {
					if (periodFound) {
						return false;
					}
					periodFound = true;
					return true;
				}
				// Explicit casting avoids problems such as non-ASCII characters being promoted to ints.
				return static_cast<bool>(std::isdigit(static_cast<unsigned char>(c)));
			})) {
				throw std::runtime_error("bond return must be a number");
			}

			try {
				std::string s;
				s.reserve(sv.size() + 1);
				// Add back the minus sign if negative.
				if (isNegative) {
					s.push_back('-');
				}
				s.append(sv);
				// Attempt to convert the resulting string to a double using std::stod.
				return std::stod(s);
			}
			catch (const std::out_of_range&) {
				throw std::runtime_error("bond return is too large");
			}
			catch (const std::invalid_argument&) {
				throw std::runtime_error("bond return must be a number");
			}
		}
	}

	/**
	* Takes a provided file path string, checks the file contains bond return data in the required format,
	* and returns the data as BondReturnData.
	*/
	BondReturnData loadBondReturnCSV(const std::string& filePath) {
		// VALIDATE FILE -----------------------------------------------------------------------------------------------

		const std::string fileExtension = InputHelpers::getExtension(filePath);

		// Warn the user explicitly if they try to provide a spreadsheet.
		if (Detail::isSpreadsheetExtension(fileExtension)) {
			throw FileError(std::format("file extension .{} is a spreadsheet format, save as CSV instead", fileExtension));
		}

		// Warn less specifically if they provided anything other than a .csv or .txt file.
		if (fileExtension != "csv" && fileExtension != "txt") {
			throw FileError(std::format("file extension must be .csv or .txt, received .{}", fileExtension));
		}

		std::ifstream fileData(filePath);

		// Check the file exists and can be opened.
		if (!fileData) {
			throw FileError(std::format("cannot open {}", filePath));
		}

		// Check the file is non-empty.
		std::string currentRow;
		if (!std::getline(fileData, currentRow)) {
			throw FileError(std::format("{} is empty", filePath));
		}

		// Get a string_view of the first non-empty row, throwing if none found.
		std::string_view currentRowView;
		do {
			currentRowView = std::string_view(currentRow);
		}
		while (Detail::isBlankLineCSV(currentRowView) && std::getline(fileData, currentRow));

		if (Detail::isBlankLineCSV(currentRowView)) {
			throw FileError("all lines blank");
		}

		// VALIDATE HEADER ---------------------------------------------------------------------------------------------

		// Get the header row values, splitting the row data at ','.
		auto rowCells = Detail::getCells(currentRowView);

		// Verify the first column heading is "Tenor" to help ensure that the user understands the data specification.
		std::string_view firstCell = *rowCells.begin();
		GeneralHelpers::svTrimWhitespace(firstCell);
		if (GeneralHelpers::svToLowercase(firstCell) != "tenor") {
			throw FileError(std::format("First entry should be \"Tenor\", received {}", firstCell));
		}

		// Check that there are no missing months, and count the number provided.
		std::size_t currentMonth = 0;
		for (auto cell : rowCells | std::views::drop(1)) {
			GeneralHelpers::svTrimWhitespace(cell);
			if (std::string(cell) != std::to_string(currentMonth)) {
				throw FileError(std::format("missing or mislabelled month {}", currentMonth));
			}
			++currentMonth;
		}
		if (currentMonth == 0) {
			throw FileError("no bond return data");
		}
		std::size_t numMonths = currentMonth;

		// READ ROWS ---------------------------------------------------------------------------------------------------

		std::vector<int> tenorsUnsorted; // the unsorted list of tenors (i.e. the data from the first column)
		std::vector<double> gridUnsorted; // a row-major vectorisation of the bond data grid as given

		// tenorSeen is used to detect duplicate tenors,
		// testing insertion into an unordered set is faster than checking vector membership.
		std::unordered_set<int> tenorSeen;

		for (std::size_t rowNum = 2; std::getline(fileData, currentRow); ++rowNum) {
			// Skip blank lines.
			currentRowView = std::string_view(currentRow);
			if (Detail::isBlankLineCSV(currentRow)) {
				continue;
			}

			// Get the current row values, splitting the row data at ','.
			rowCells = Detail::getCells(currentRowView);

			// Read and validate the tenor of the current row, add it to the tenor list if valid, throw if not.
			int currentTenor;
			try {
				currentTenor = Detail::parseTenor(*rowCells.begin());
			}
			catch (const std::runtime_error &e) {
				throw FileError(std::format("row {}: {}", rowNum, e.what()));
			}
			if (!tenorSeen.insert(currentTenor).second) {
				throw FileError(std::format("row {}: duplicate tenor {}", rowNum, currentTenor));
			}
			tenorsUnsorted.push_back(currentTenor);

			// Add another row's worth of space to the grid vector.
			const std::size_t base = gridUnsorted.size();
			gridUnsorted.resize(base + numMonths);

			// Traverse the row, keeping track of the month. Read and validate the bond return for the current tenor
			// and month, add it to the bond return data grid if valid, throw if not.
			currentMonth = 0;
			for (auto cell : rowCells | std::views::drop(1)) {
				GeneralHelpers::svTrimWhitespace(cell);
				if (cell.empty()) {
					throw FileError(std::format("row {}, column {}: missing bond return", rowNum, currentMonth));
				}
				try {
					gridUnsorted[base + currentMonth] = Detail::parseBondReturn(cell);
				}
				catch (const std::runtime_error &e) {
					throw FileError(std::format("row {}, column {}: {}", rowNum, currentMonth, e.what()));
				}
				++currentMonth;
			}

			// Check that there are no missing months of bond return data for this tenor.
			if (currentMonth != numMonths) {
				if (currentMonth == numMonths - 1) {
					throw FileError(std::format("row {}: missing column {}", rowNum, numMonths - 1));
				}
				throw FileError(std::format("row {}: missing columns {} to {}", rowNum, currentMonth, numMonths - 1));
			}
		}

		// SORT ROWS ---------------------------------------------------------------------------------------------------

		// sortedIndices stores the indices of the tenors in ascending order.
		// For example, if tenorsUnsorted were { 3, 9, 6 }, sortedIndices would be { 0, 2, 1 }.
		std::vector<std::size_t> sortedIndices(tenorsUnsorted.size());
		// std::iota sets sortedIndices to { 0, 1, ..., tenorsUnsorted.size() - 1 }
		std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
		// We then sort these indices according to the values of tenorsUnsorted of the corresponding index.
		std::ranges::sort(sortedIndices, [&](const std::size_t a, const std::size_t b) {
			return tenorsUnsorted[a] < tenorsUnsorted[b];
		});

		// If we have fewer months of data than the shortest tenor, no solution is possible.
		if (const int shortestTenor = tenorsUnsorted[sortedIndices.front()];
			numMonths < static_cast<std::size_t>(shortestTenor))
		{
			throw FileError(std::format("shortest tenor is {} months, but only {} months of data provided",
				shortestTenor, numMonths - 1));
		}

		std::vector<int> tenorsSorted(tenorsUnsorted.size()); // list of tenors in ascending order
		std::vector<double> gridSorted(gridUnsorted.size());  // a row-major vectorisation of the bond data grid sorted by tenor

		// We now construct the sorted tenor list and grid.
		for (std::size_t r = 0; r < sortedIndices.size(); ++r) {
			const std::size_t sourceIndex = sortedIndices[r]; // the index in tenorsUnsorted of the rth tenor in ascending order
			tenorsSorted[r] = tenorsUnsorted[sourceIndex]; // stores the rth tenor in ascending order

			// Copy numMonths of bond return data from gridUnsorted starting at the position corresponding to the values
			// for tenorsSorted[r] into the position corresponding to the rth "row" of the vectorised gridSorted.
			std::copy_n(&gridUnsorted[sourceIndex * numMonths], numMonths, &gridSorted[r * numMonths]);
		}

		return BondReturnData{ std::move(tenorsSorted), numMonths, std::move(gridSorted) };
	}
}