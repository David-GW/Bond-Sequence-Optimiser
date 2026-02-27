#include "app/io/CSVLoader.hpp"

#include "app/domain/BondReturnData.hpp"
#include "helpers/Filesystem.hpp"
#include "helpers/Strings.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace IO::Input
{
	namespace Detail
	{
		constexpr std::string_view tooManyRowsMessage = "CSV too large: too many rows provided";

		[[nodiscard]] static constexpr bool isBlankLineCSV(std::string_view sv) noexcept {
			return std::ranges::all_of(sv, [](const unsigned char c) {
				return std::isspace(c) || c == ',';
			});
		}

		/// Takes a row of CSV data and returns string_views over the cells split at ','.
		[[nodiscard]] static constexpr auto getCells(std::string_view row) noexcept {
			return row | std::views::split(',') | std::views::transform([](auto&& v) {
				return std::string_view{std::ranges::begin(v), std::ranges::end(v)};
			});
		}

		namespace Filepath
		{
			/// Checks if an extension matches that of common spreadsheet formats,
			/// since a user may mistake this for a CSV file.
			[[nodiscard]] static constexpr bool isSpreadsheetExtension(const std::string_view ext) noexcept {
				constexpr auto spreadsheetExtensions = std::to_array<std::string_view>({
					"xlsx", "xls", "xlsm", "xlsb", "numbers", "ods"
				});
				return std::ranges::contains(spreadsheetExtensions, ext);
			}

			/// Returns the fully-expanded CSV path, checking its validity and extension
			[[nodiscard]] static std::filesystem::path validatedPath(const std::string_view CSVPathSv) {
				// Expand and check path:
				std::filesystem::path CSVPath{};
				try {
					CSVPath = Helpers::Filesystem::expandUserPath(CSVPathSv);
					Helpers::Filesystem::assertDirectoryValid(Helpers::Filesystem::getDirectory(CSVPath));
					Helpers::Filesystem::assertFileValid(CSVPath);
				}
				catch (const Helpers::Filesystem::FilesystemError& e) {
					throw CSVError(e.what());
				}

				// Check extension:
				const std::string fileExtension = Helpers::Strings::svToLowercase(Helpers::Filesystem::getExtension(CSVPath));
				if (fileExtension.empty()) {
					throw CSVError("file has no extension, must be .csv or .txt");
				}
				if (isSpreadsheetExtension(fileExtension)) {
					throw CSVError(
						std::format("file extension .{} is a spreadsheet format, save as CSV instead", fileExtension)
					);
				}
				if (fileExtension != "csv" && fileExtension != "txt") {
					throw CSVError(std::format("file extension must be .csv or .txt, received .{}", fileExtension));
				}

				return CSVPath;
			}
		}

		namespace Header
		{
			struct HeaderData
			{
				std::string headerContents{};
				int headerRowNum{};
			};

			/// Gets the contents and line number of the first non-empty row,
			/// throwing a CSVError if none found or if too many rows provided:
			[[nodiscard]] static HeaderData getHeaderData(std::ifstream& CSVStream) {
				std::string currentRow{};
				int currentRowNum = 0;

				while (std::getline(CSVStream, currentRow)) {
					if (currentRowNum == std::numeric_limits<int>::max()) {
						throw CSVError(std::string{tooManyRowsMessage});
					}
					++currentRowNum;

					if (const std::string_view currentRowView{currentRow}; isBlankLineCSV(currentRowView)) {
						continue;
					}

					return {.headerContents = std::move(currentRow), .headerRowNum = currentRowNum};
				}

				throw CSVError("all lines blank");
			}

			/// Verifies the validity of the header and returns the number of months provided.
			[[nodiscard]] static int numMonthsInHeader(const std::string_view headerView) {
				auto rowCells = getCells(headerView);

				// Verify the first column heading is "Tenor"
				// to help ensure that the user understands the data specification.
				std::string_view firstCell = *rowCells.begin();
				Helpers::Strings::svTrimWhitespaceInPlace(firstCell);
				if (Helpers::Strings::svToLowercase(firstCell) != "tenor") {
					throw CSVError(std::format("first entry should be \"Tenor\", received {}", firstCell));
				}

				// Check that there are no missing months, and count the number provided:
				int currentMonth = 0;
				for (auto cell : rowCells | std::views::drop(1)) {
					if (currentMonth == std::numeric_limits<int>::max()) {
						throw CSVError("CSV too large: too many months provided");
					}
					Helpers::Strings::svTrimWhitespaceInPlace(cell);
					int parsed{};
					if (
						auto [ptr, ec] = std::from_chars(cell.data(), cell.data() + cell.size(), parsed);
						static_cast<bool>(ec) || ptr != cell.data() + cell.size() || parsed != currentMonth
					) {
						throw CSVError(
							std::format("missing or mislabelled month {}: found {}", currentMonth, cell)
						);
					}
					++currentMonth;
				}
				if (currentMonth == 0) {
					throw CSVError("no bond return data");
				}
				return currentMonth;
			}
		}

		namespace Parsing
		{
			/// Thrown if an error occurs parsing a tenor or bond return.
			struct ParseError final : std::runtime_error
			{
				using std::runtime_error::runtime_error;
			};

			[[nodiscard]] static int parseTenor(std::string_view sv) {
				Helpers::Strings::svTrimWhitespaceInPlace(sv);
				if (sv.empty()) {
					throw ParseError("missing tenor");
				}

				int result{};
				auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);

				const std::string mustBePositive = "tenor must be a positive integer";
				if (ec == std::errc::result_out_of_range) {
					if (sv.front() == '-') {
						throw ParseError(mustBePositive);
					}
					throw ParseError("tenor is too long");
				}
				if (ec == std::errc::invalid_argument || ptr != sv.data() + sv.size()) {
					throw ParseError("invalid tenor");
				}
				if (result <= 0) {
					throw ParseError(mustBePositive);
				}
				return result;
			}

			[[nodiscard]] static double parseBondReturn(std::string_view sv) {
				Helpers::Strings::svTrimWhitespaceInPlace(sv);
				if (sv.empty()) {
					throw ParseError("missing bond return");
				}

				double result{};
				auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);

				constexpr std::string_view returnTooSmallMessage = "bond return is too small";
				constexpr std::string_view returnTooLargeMessage = "bond return is too large";
				if (ec == std::errc::result_out_of_range) {
					if (sv.front() == '-') {
						throw ParseError(std::string(returnTooSmallMessage));
					}
					throw ParseError(std::string(returnTooLargeMessage));
				}
				if (ec == std::errc::invalid_argument || ptr != sv.data() + sv.size() || std::isnan(result)) {
					throw ParseError("invalid bond return");
				}
				// We multiply by (1 + return %) when computing returns,
				// and so need to check if this is infinite, rather than just the return % itself.
				if (const double onePlus = 1.0 + result; std::isinf(onePlus)) {
					if (std::signbit(onePlus)) {
						throw ParseError(std::string(returnTooSmallMessage));
					}
					throw ParseError(std::string(returnTooLargeMessage));
				}
				return result;
			}
		}

		namespace Data
		{
			struct UnsortedData
			{
				// Unsorted list of tenors (i.e. the data from the first column).
				std::vector<int> tenorsUnsorted{};
				// Row-major vectorisation of the bond data grid in CSV.
				std::vector<double> gridUnsorted{};
			};

			/// Loads data from the CSV stream into an UnsortedData struct.
			[[nodiscard]] static UnsortedData loadData(std::ifstream& CSVStream, int currentRowNum, int numMonths) {
				std::vector<int> tenorsUnsorted{};
				std::vector<double> gridUnsorted{};

				// Used to detect duplicate tenors, testing unordered set membership is faster than checking vector membership.
				std::unordered_set<int> tenorsSeen{};

				std::string currentRow{};
				while (std::getline(CSVStream, currentRow)) {
					if (currentRowNum == std::numeric_limits<int>::max()) {
						throw CSVError(std::string{tooManyRowsMessage});
					}
					++currentRowNum;

					// Skip blank lines:
					auto currentRowView = std::string_view{currentRow};
					if (isBlankLineCSV(currentRowView)) {
						continue;
					}

					auto currentRowNumString = std::to_string(currentRowNum);

					auto rowCells = getCells(currentRowView);

					// Parse tenor:
					int currentTenor{};
					try {
						currentTenor = Parsing::parseTenor(*rowCells.begin());
					}
					catch (const Parsing::ParseError &e) {
						throw CSVError(std::format("row {}: {}", currentRowNumString, e.what()));
					}
					if (!tenorsSeen.emplace(currentTenor).second) {
						throw CSVError(
							std::format("row {}: duplicate tenor {}", currentRowNumString, currentTenor)
						);
					}
					tenorsUnsorted.push_back(currentTenor);

					// Add another row's worth of space to the grid vector:
					const std::size_t base = gridUnsorted.size();
					gridUnsorted.resize(base + static_cast<std::size_t>(numMonths));

					// Traverse the row, read and validate the bond return for the current tenor and month,
					// add it to the bond return data grid if valid, throw a CSVError if not:
					int currentMonth = 0;
					for (auto cell : rowCells | std::views::drop(1)) {
						try {
							gridUnsorted[base + static_cast<std::size_t>(currentMonth)] = Parsing::parseBondReturn(cell);
						}
						catch (const Parsing::ParseError &e) {
							throw CSVError(
								std::format("row {}, month {}: {}", currentRowNumString, currentMonth, e.what())
							);
						}
						++currentMonth;
					}

					// Check that there are no missing months of bond return data for this tenor:
					if (currentMonth != numMonths) {
						if (currentMonth == numMonths - 1) {
							throw CSVError(
								std::format("row {}: missing month {}", currentRowNumString, numMonths - 1)
							);
						}
						throw CSVError(
							std::format(
								"row {}: missing months {} to {}",
								currentRowNumString,
								currentMonth,
								numMonths - 1
							)
						);
					}
				}
				if (tenorsUnsorted.empty()) {
					throw CSVError("no bond return data");
				}
				return {.tenorsUnsorted = std::move(tenorsUnsorted), .gridUnsorted = std::move(gridUnsorted)};
			}
		}

		namespace Sorting
		{
			struct SortedData
			{
				std::vector<int> tenorsSorted{};
				std::vector<double> gridSorted{};
			};

			/// Returns sorted copies of the tenor list and return grid in case the CSV data is disordered
			[[nodiscard]] static SortedData sortData(
				const std::vector<int>& tenorsUnsorted,
				const std::vector<double>& gridUnsorted,
				const int numMonths
			) {
				// sortedIndices stores the indices of the tenors in ascending order,
				// for example, if tenorsUnsorted were { 3, 9, 6 }, then sortedIndices would be { 0, 2, 1 }.
				std::vector<std::size_t> sortedIndices(tenorsUnsorted.size());
				// std::iota sets sortedIndices to { 0, 1, ..., tenorsUnsorted.size() - 1 }
				std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
				// We then sort these indices according to the values of tenorsUnsorted of the corresponding index.
				std::ranges::sort(sortedIndices, {}, [&](const std::size_t i) { return tenorsUnsorted[i]; });

				// If we have fewer months of data than the shortest tenor, no solution is possible.
				if (const int shortestTenor = tenorsUnsorted[sortedIndices.front()]; numMonths < shortestTenor) {
					throw CSVError(
						std::format(
							"shortest tenor is {} months, but only {} months of data provided",
							shortestTenor,
							numMonths - 1
						)
					);
				}

				std::vector<int> tenorsSorted(tenorsUnsorted.size());
				std::vector<double> gridSorted(gridUnsorted.size());

				// Construct the sorted tenor list and grid:
				for (std::size_t r = 0; r < sortedIndices.size(); ++r) {
					// The index in tenorsUnsorted of the rth tenor (in ascending order).
					const std::size_t sourceIndex = sortedIndices[r];
					// Store the rth tenor (in ascending order):
					tenorsSorted[r] = tenorsUnsorted[sourceIndex];

					// Copy numMonths of bond return data from gridUnsorted starting at the position corresponding to
					// the values for tenorsSorted[r] into the position corresponding to the rth "row" of the
					// vectorised gridSorted:
					std::copy_n(&gridUnsorted[sourceIndex * numMonths], numMonths, &gridSorted[r * numMonths]);
				}

				return {.tenorsSorted = std::move(tenorsSorted), .gridSorted = std::move(gridSorted)};
			}
		}
	}

	Domain::BondReturnData loadBondReturnCSV(const std::string_view CSVPathSv) {
		// Open file:
		auto CSVPath = Detail::Filepath::validatedPath(CSVPathSv);
		std::ifstream CSVStream{CSVPath};
		if (!CSVStream.is_open()) {
			throw CSVError(std::format("cannot open\n{}", CSVPath.string()));
		}
		if (CSVStream.peek() == std::ifstream::traits_type::eof()) {
			throw CSVError(std::format("{}\nis empty", CSVPath.string()));
		}

		// Read header:
		auto headerData = Detail::Header::getHeaderData(CSVStream);
		auto headerView = std::string_view{headerData.headerContents};
		int numMonths = Detail::Header::numMonthsInHeader(headerView);

		// Load data:
		auto unsortedData = Detail::Data::loadData(CSVStream, headerData.headerRowNum, numMonths);
		if (!CSVStream.eof()) {
			throw CSVError(std::format("error reading\n{}", CSVPath.string()));
		}

		// Sort data:
		auto sortedData = Detail::Sorting::sortData(unsortedData.tenorsUnsorted, unsortedData.gridUnsorted, numMonths);

		return Domain::BondReturnData{
			std::move(sortedData.tenorsSorted), numMonths, std::move(sortedData.gridSorted), std::move(CSVPath)
		};
	}
}