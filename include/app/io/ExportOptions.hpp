#ifndef BSO_APP_IO_EXPORT_OPTIONS_HPP
#define BSO_APP_IO_EXPORT_OPTIONS_HPP

#include <filesystem>
#include <string_view>
#include <variant>

namespace Domain
{
	class BondReturnData;
}

namespace IO::Output
{
	// If the user decides to save the results, the file will be "RESULTS_FILENAME.csv"
	// or some "RESULTS_FILENAME_<num>.csv".
	constexpr std::string_view RESULTS_FILENAME = "bond_results";
	// We set a limit to avoid, for example, the existence of "RESULTS_FILENAME_{1 to (MAX_INT)}.csv" causing problems,
	// despite this situation being unlikely it does defend against overflows.
	constexpr int RESULT_FILES_LIMIT = 10'000;

	namespace Decision
	{
		struct Save
		{
			std::filesystem::path filePath{};
		};
		struct Print {};
		struct Quit {};
	}
	/// The user can decide to save to disk (in which case the chosen path is stored), print to terminal, or abort.
	using ExportDecision = std::variant<Decision::Save, Decision::Print, Decision::Quit>;

	/// Prompts the user to decide how to export the result.
	[[nodiscard]] ExportDecision getExportDecision(const Domain::BondReturnData& tenorData);
}

#endif // BSO_APP_IO_EXPORT_OPTIONS_HPP