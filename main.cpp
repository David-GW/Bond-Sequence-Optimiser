#include "app/cli/Prompts.hpp"
#include "app/counter/PathCounter.hpp"
#include "app/domain/BondReturnData.hpp"
#include "app/domain/InvestmentAction.hpp"
#include "app/io/ExportOptions.hpp"
#include "app/io/ResultsOutput.hpp"
#include "app/optimiser/DynamicOptimiser.hpp"
#include "helpers/DistinguishedVariant.hpp"
#include "helpers/Meta.hpp"
#include "helpers/Strings.hpp"
#include "helpers/printing/StyledPrint.hpp"
#include "transformers/Generic.hpp"
#include "transformers/Mapping.hpp"

#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <print>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

int main()
{
// INPUT ---------------------------------------------------------------------------------------------------------------

	int numResultsRequested{};
	std::size_t numResultsFound{};

	std::vector<int> tenorList{};
	int numMonths{};

	std::chrono::duration<double, std::milli> computationTime{};

	try {
		const auto dataPromptResult = Prompts::getDataPrompt();
		if (dataPromptResult.isEscape()) {
			return 0;
		}
		const auto& tenorData = dataPromptResult.getValue();
		std::println();

		tenorList = tenorData.tenors();
		numMonths = tenorData.numMonths();

		const auto numResultsPromptResult = Prompts::getNumResultsPrompt();
		if (numResultsPromptResult.isEscape()) {
			return 0;
		}
		numResultsRequested = numResultsPromptResult.getValue();
		std::println();

		const auto exportDecision = IO::Output::getExportDecision(tenorData);

// CALCULATION ---------------------------------------------------------------------------------------------------------

		const auto startTime = std::chrono::steady_clock::now();

		DynamicOptimiser::OptimalResults results{};
		try {
			results = DynamicOptimiser::getOptimalSequences(tenorData, numResultsRequested);
		}
		catch (const std::overflow_error& e) {
			Helpers::Printing::styledPrintln(Helpers::Printing::Styles::error, "Overflow: {}", e.what());
			return 1;
		}

		const auto endTime = std::chrono::steady_clock::now();

		computationTime = endTime - startTime;

		numResultsFound = results.CRFs.size();

// OUTPUT --------------------------------------------------------------------------------------------------------------

		const auto outcome = std::visit(
			Helpers::Meta::overloaded(
				[&](const IO::Output::Decision::Save& d) {
					return IO::Output::exportCSV(results, numResultsFound, d.filePath);
				},
				[](const IO::Output::Decision::Print&) {
					return IO::Output::ExportOutcome::Print;
				},
				[](const IO::Output::Decision::Quit&) {
					return IO::Output::ExportOutcome::Quit;
				}
			),
			exportDecision
		);

		switch (outcome) {
			case IO::Output::ExportOutcome::Saved:
				break;

			case IO::Output::ExportOutcome::Print:
				IO::Output::printResults(results, numResultsFound);
				std::println();
				break;

			case IO::Output::ExportOutcome::Quit:
				return 0;
		}
	}
	catch (const std::exception& e) {
		Helpers::Printing::styledPrintln(Helpers::Printing::Styles::error, "Unexpected error: {}", e.what());
		return 1;
	}

// FINISH --------------------------------------------------------------------------------------------------------------

	if (numResultsFound < static_cast<std::size_t>(numResultsRequested)) {
		std::println(
			"Note: {} solutions requested, but only {} found",
			Helpers::Strings::formatIntWithSeparator(numResultsRequested),
			Helpers::Strings::formatIntWithSeparator(numResultsFound)
		);
		std::println();
	}

	std::println("Computation time: {:.6f} milliseconds", computationTime.count());
	std::println();

	const auto printPathCountPromptResult = Transformers::Mapping::mappingTransformer<bool>(
		"Enter \"y\" if you would like to calculate the total number of possible strategies;\n"
		"OR press ENTER to quit:",
		{{"y", true}},
		{.caseSensitive = false, .quitWord = ""}
	);
	if (printPathCountPromptResult.isEscape()) {
		return 0;
	}
	std::println();

	std::println("Total possible strategies:");
	PathCounter::printPathCount(tenorList, numMonths);
	std::println();

	std::println("Press ENTER to quit:");
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	return 0; // handles EOF / redirected input
}