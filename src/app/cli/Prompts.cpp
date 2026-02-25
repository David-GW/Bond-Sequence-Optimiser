#include "app/cli/Prompts.hpp"

#include "app/cli/OutputMessages.hpp"
#include "app/domain/BondReturnData.hpp"
#include "app/io/CSVLoader.hpp"
#include "helpers/Quit.hpp"
#include "helpers/Strings.hpp"
#include "transformers/Generic.hpp"
#include "transformers/Mapping.hpp"
#include "transformers/Numeric.hpp"

#include <format>
#include <string_view>

namespace Prompts
{
	DataPromptResult getDataPrompt() {
		return Transformers::promptTransformer(
			"Enter the path to your bond return data file (e.g. bond_data.csv or txt);\n"
			"OR enter 'h' to show file help;\n"
			"OR press ENTER to quit:",
			[](const std::string_view sv) -> Transformers::TransformerResult<Domain::BondReturnData> {
				if (sv.empty()) {
					if (Helpers::Quit::confirmQuit()) {
						return Transformers::Escape();
					}
					return Transformers::Retry();
				}
				if (Helpers::Strings::svCaseInsensitiveCompare(sv, "h")) {
					OutputMessages::printFileHelp();
					return Transformers::Retry();
				}
				try {
					return IO::Input::loadBondReturnCSV(sv);
				}
				catch (const IO::Input::CSVError& e) {
					return Transformers::Retry(std::format("Failed to load data: {}", e.what()));
				}
			}
		);
	}

//----------------------------------------------------------------------------------------------------------------------

	NumResultsPromptResult getNumResultsPrompt() {
		while (true) {
			const auto numResultsPromptResult = Transformers::Numeric::positiveIntTransformer(
				"Enter how many of the top results you would like;\n"
				"OR press ENTER to quit:"
			);
			if (numResultsPromptResult.isEscape()) {
				return numResultsPromptResult;
			}

			if (
				const int numResultsRequested = numResultsPromptResult.getValue();
				// Warning number set in header.
				numResultsRequested > REQUEST_WARNING_NUM
			) {
				std::println();
				const auto printFallbackPromptResult = Transformers::Mapping::mappingTransformer<bool>(
					std::format(
						"WARNING: You have requested a large number of results ({}).\n"
						"Enter \"y\" to proceed anyway;\n"
						"OR press ENTER to input a new value:",
						Helpers::Strings::formatIntWithSeparator(numResultsRequested)
					),
					{{"y", true}},
					{.caseSensitive = false, .quitWord = ""}
				);
				if (printFallbackPromptResult.isEscape()) {
					continue;
				}
			}

			return numResultsPromptResult;
		}
	}
}
