#include "app/io/ResultsOutput.hpp"

#include "app/domain/InvestmentAction.hpp"
#include "app/optimiser/DynamicOptimiser.hpp"
#include "helpers/Strings.hpp"
#include "helpers/printing/StyledPrint.hpp"
#include "transformers/Generic.hpp"
#include "transformers/Mapping.hpp"

#include <cstddef>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace IO::Output
{
	namespace Detail
	{
		[[nodiscard]] static std::string formatCSVRow(
			const DynamicOptimiser::OptimalResults& results,
			const std::size_t i
		) {
			return std::format(
				"{},{:.2f}%,\"{}\"",
				i + 1,
				100 * results.CRFs[i] - 100,
				Helpers::Strings::joinFormatted(results.decisions[i], ",")
			);
		}
	}

	void printResults(const DynamicOptimiser::OptimalResults& results, const std::size_t numResultsToPrint) {
		std::println();
		std::println("Results:");
		std::println();
		for (std::size_t i = 0; i < numResultsToPrint; ++i) {
			std::println(
				"{}. {:.2f}%: {}",
				i + 1,
				100 * results.CRFs[i] - 100,
				Helpers::Strings::joinFormatted(results.decisions[i],",")
			);
		}
	}

	ExportOutcome exportCSV(
		const DynamicOptimiser::OptimalResults& results,
		const std::size_t numResultsToExport,
		const std::filesystem::path& filePath
	) {
		try {
			std::ofstream out(filePath, std::ios::trunc);
			// Convert stream state failures into exceptions so partial/failed writes are handled by the catch path:
			out.exceptions(std::ofstream::failbit | std::ofstream::badbit);

			for (std::size_t i = 0; i < numResultsToExport; ++i) {
				if (i > 0) {
					out << '\n';
				}
				out << Detail::formatCSVRow(results, i);
			}
			out.flush();

			std::println("Export complete, saved to:");
			std::println("{}", filePath.string());
			std::println();

			return ExportOutcome::Saved;
		}
		catch (const std::ios_base::failure&) {
			Helpers::Printing::styledPrintln(
				Helpers::Printing::Styles::error, "Failed to write to {}", filePath.string()
			);
			std::println();
			// Give the option to print if write fails:
			const auto printFallbackPromptResult = Transformers::Mapping::mappingTransformer<bool>(
				"Enter \"p\" to print results to the terminal;\n"
				"OR press ENTER to abort:",
				{{"p", true}},
				{.caseSensitive = false, .quitWord = "abort"}
			);
			if (printFallbackPromptResult.isEscape()) {
				return ExportOutcome::Quit;
			}
			return ExportOutcome::Print;
		}
	}
}