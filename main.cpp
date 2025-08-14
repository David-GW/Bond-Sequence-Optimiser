#include "helpers/GeneralHelpers.h"
#include "helpers/InputHelpers.h"

#include "optimisers/DynamicOptimiser.h"
#include "optimisers/RecursiveOptimiser.h"

#include "types/BondReturnData.h"
#include "types/CRFAndChoices.h"
#include "types/InvestmentAction.h"
#include "types/RankingTypes.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <tuple>
#include <vector>

namespace OutputMessages
{
	/// Defines an error to be thrown if, for whatever reason, no viable combination of tenor choices can be found.
	struct NoSolutionsError final : std::exception {
		[[nodiscard]] const char* what() const noexcept override {
			return "no solutions found";
		}
	};

	/// Prints an ExtremeList of top/bottom results, accounting for the actual number of solutions found.
	void printExtremeResults(const ExtremeList& extremeList, const int numSolutions) {
		if (numSolutions == 0) {
			throw NoSolutionsError();
		}

		const int numToPrint = static_cast<int>(extremeList.listLength());
		const auto& elemList = extremeList.rankedList();
		const double defaultValue = extremeList.listDefault();

		if (numToPrint > 0) {
			if (numToPrint == 1) {
				std::println("{} cumulative return and tenor choices:", extremeList.rankingType());
				std::println("{}", elemList.front());
			}
			else {
				// It may be that the user asks for more results than there are solutions.
				const int actualPrintNum = std::min(numToPrint, numSolutions);

				// Begin printing results.
				std::println("{} {} cumulative returns and tenor choices:", extremeList.rankingType(), actualPrintNum);
				for (const auto& choice : elemList) {
					// If we reach the default value of this list, we know we need to stop, there are no more solutions.
					if (choice.CRF() == defaultValue) break;

					std::println("{}", choice);
				}

				// Notify the user if they requested too many results.
				if (actualPrintNum < numToPrint) {
					if (actualPrintNum == 1) {
						std::println("NOTE: {} {} results requested, but only 1 solution exists",
							extremeList.rankingType(), numToPrint);
					}
					else {
						std::println("NOTE: {} {} results requested, but only {} solutions exist",
							extremeList.rankingType(), numToPrint, actualPrintNum);
					}
				}
			}
		}
	}

	/// Prints an example of the format of the bond data file.
	void printExampleCSV() {
		std::println("Tenor, 0, 1, 2, ...");
		std::println("3, 0.03197, 0.03225, 0.03179, ...");
		std::println("6, 0.06517, 0.06606, 0.06554, ...");
		std::println("...");
	}

	/// Prints an example of how the  bond data file should look if opened in spreadsheet software.
	void printExampleTable() {
		std::println("  Tenor  |    0    |    1    |    2    |  ...   ");
		std::println("---------+---------+---------+---------+--------");
		std::println("    3    | 0.03197 | 0.03225 | 0.03179 |  ...   ");
		std::println("---------+---------+---------+---------+--------");
		std::println("    6    | 0.06517 | 0.06606 | 0.06554 |  ...   ");
		std::println("---------+---------+---------+---------+--------");
		std::println("   ...   |         |         |         |        ");
	}

	/// Print comprehensive instructions on the required format of the bond data file.
	void printFileHelp() {
		std::println();
		OutputHelpers::printRule();
		std::println("FILE HELP");
		OutputHelpers::printRule();
		std::println();
		OutputHelpers::wrappedPrint("Bond return data should be provided as a CSV file (a .csv or .txt extension is required).");
		std::println();
		OutputHelpers::wrappedPrint("The first row should have as its first value the word \"Tenor\", "
			"followed by a consecutive list of months starting at 0.");
		std::println();
		OutputHelpers::wrappedPrint("Subsequent rows should begin with the tenor, and then list the return "
			"should a bond of that tenor be purchased in the month of the corresponding column.");
		std::println();
		std::println("For example, the CSV file should resemble:");
		std::println();
		printExampleCSV();
		std::println();
		std::println("(spaces are optional, and blank rows will be ignored).");
		std::println();
		OutputHelpers::wrappedPrint("If opened in spreadsheet software such as Excel, the data should resemble:");
		std::println();
		printExampleTable();
		std::println();
		OutputHelpers::wrappedPrint("but, if editing in such software, ensure that the file remains saved as .csv or .txt.");
		std::println();
		OutputHelpers::printRule();
	}
}

int main()
{
	std::println();
	const auto algoTypeOpt = InputHelpers::promptValidated<int>(
		"Enter 0 for the optimal cumulative return and corresponding buying strategy;\n"
		"Enter 1 to choose how many top and/or bottom results to display;\n"
		"OR press ENTER to quit:",
		[](const std::string_view x){ return x == "0" || x == "1"; },
		"Invalid entry"
	);
	if (!algoTypeOpt) {
		return 0;
	}

	const bool useDP = *algoTypeOpt == 0;
	std::println();

	if (!useDP) {
		std::println("NOTE:");
		OutputHelpers::wrappedPrint("The DFS recursive algorithm used to find any number of top and/or bottom results "
			"cannot account for waiting (unlike the dynamic programming algorithm returning only the optimal result). That is, "
			"there can be no periods between bond purchases.");
		std::println();
	}

	std::string filePath;
	std::optional<BondReturnData> dataOpt;
	bool validFileEntered = false;
	while (!validFileEntered) {
		std::println("Enter the path to your bond return data file (e.g. bond_data.csv or txt);");
		std::println("OR enter 'h' to show file help;");
		std::println("OR press ENTER to quit:");
		std::getline(std::cin, filePath);

		if (filePath.empty()) {
			return 0;
		}

		if (filePath.size() == 1 && GeneralHelpers::svToLowercase(filePath) == "h") {
			OutputMessages::printFileHelp();
			std::println();
			continue;
		}

		try {
			dataOpt.emplace(CSVLoader::loadBondReturnCSV(filePath));
		}
		catch (const CSVLoader::FileError& e) {
			std::println(std::cerr, "Failed to load data: {}", e.what());
			std::println();
			continue;
		}
		catch (const std::exception& e) {
			std::println(std::cerr, "Error: {}", e.what());
			std::println();
			return 1;
		}

		validFileEntered = true;
	}

	const BondReturnData& tenorData = *dataOpt;
	std::println();

	try {
		std::chrono::steady_clock::time_point start;
		if (useDP) {
			start = std::chrono::steady_clock::now();
			auto [ optimalCRF, optimalChoices ] = DynamicOptimiser::optimiseCRF(tenorData);

			std::println("Optimal cumulative return: {:.2f}%", 100 * optimalCRF - 100);
			for (const InvestmentAction& choice : optimalChoices) {
				std::println("{}", choice);
			}
		}
		else {
			const auto numTopOpt = InputHelpers::promptValidated<int>(
				"Enter how many of the top results you would like; OR press ENTER to quit:",
				GeneralHelpers::svAllDigits,
				"Entry must be a positive integer"
			);
			if (!numTopOpt) {
				return 0;
			}
			const int numTop = *numTopOpt;
			std::println();

			const auto numBotOpt = InputHelpers::promptValidated<int>(
				"Enter how many of the bottom results you would like; OR press ENTER to quit:",
				GeneralHelpers::svAllDigits,
				"Entry must be a positive integer"
			);
			if (!numBotOpt) {
				return 0;
			}
			const int numBot = *numBotOpt;
			std::println();

			start = std::chrono::steady_clock::now();

			if (!(numTop == 0 && numBot == 0)) {
				const auto [topChoices, botChoices, numSolutions] =
					RecursiveOptimiser::topBotCRFs(tenorData, numTop, numBot);

				try {
					OutputMessages::printExtremeResults(topChoices, numSolutions);
					std::println();
					OutputMessages::printExtremeResults(botChoices, numSolutions);
					std::println();
					std::println("Total results: {}", numSolutions);
				}
				catch (const OutputMessages::NoSolutionsError& e) {
					std::println(std::cerr, "Error: {}", e.what());
				}
			}
		}
		const auto end = std::chrono::steady_clock::now();
		const std::chrono::duration<double> elapsed = end - start;
		std::println();
		std::println("Elapsed time: {:.6f} milliseconds", 1000 * elapsed.count());
		std::println();
	}
	catch (const std::exception& e) {
		std::println(std::cerr, "Error: {}", e.what());
		return 1;
	}

	std::println("Press ENTER to quit.");
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	return 0;
}
