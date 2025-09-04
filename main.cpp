#include "cli/OutputMessages.h"

#include "helpers/GeneralHelpers.h"
#include "helpers/InputHelpers.h"
#include "helpers/OutputHelpers.h"

#include "optimisers/DynamicOptimiser.h"
#include "optimisers/RecursiveOptimiser.h"

#include "types/BondReturnData.h"
#include "types/CRFAndChoices.h"
#include "types/InvestmentAction.h"
#include "types/RankingTypes.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

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
