#ifndef BSO_APP_IO_RESULTS_OUTPUT_HPP
#define BSO_APP_IO_RESULTS_OUTPUT_HPP

#include "app/optimiser/DynamicOptimiser.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace IO::Output
{
	/// Stores what actually happened after the user made their export decision, since writes can fail.
	enum class ExportOutcome { Saved, Print, Quit };

	/// Prints the optimiser results to the terminal.
	void printResults(const DynamicOptimiser::OptimalResults& results, std::size_t numResultsToPrint);

	/// Tries to save the optimiser's results to the path specified, offering to print to the terminal if writing fails.
	[[nodiscard]] ExportOutcome exportCSV(
		const DynamicOptimiser::OptimalResults& results,
		// We ask for the number of results to export rather than relying on the size of the OptimalResults object
		// since this is constructed based on how many results the user requests, it may be that fewer results exist.
		std::size_t numResultsToExport,
		const std::filesystem::path& filePath
	);
}

#endif // BSO_APP_IO_RESULTS_OUTPUT_HPP