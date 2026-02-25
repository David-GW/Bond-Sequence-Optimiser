#include "app/io/ExportOptions.hpp"

#include "app/domain/BondReturnData.hpp"
#include "helpers/Filesystem.hpp"
#include "helpers/Strings.hpp"
#include "helpers/printing/StyledPrint.hpp"
#include "transformers/Generic.hpp"
#include "transformers/Mapping.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <optional>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace IO::Output
{
	namespace Detail
	{
		constexpr std::string_view directoryUnavailableLabel = "(unavailable)";

		enum class ExportLocation { Specified, Data, Program, Terminal };

		namespace Paths
		{
			[[nodiscard]] static std::optional<std::filesystem::path> getCSVPath(
				const Domain::BondReturnData& tenorData
			) {
				std::error_code ec;
				auto canonicalPath = std::filesystem::weakly_canonical(tenorData.dataPath(), ec);
				if (!ec) {
					canonicalPath = canonicalPath.parent_path();
					return canonicalPath;
				}
				return std::nullopt;
			}

			[[nodiscard]] static std::optional<std::filesystem::path> getProgramPath() {
				std::error_code ec;
				const auto path = std::filesystem::current_path(ec);
				if (!ec) {
					auto canonicalPath = std::filesystem::weakly_canonical(path, ec);
					if (!ec) {
						return canonicalPath;
					}
				}
				return std::nullopt;
			}

			[[nodiscard]] static std::string pathOrUnavailable(const std::optional<std::filesystem::path>& path) {
				if (path) {
					return path->string();
				}
				return std::string{directoryUnavailableLabel};
			}
		}

		namespace LocationPrompt
		{
			/**
			* Asks the user if they would like to print their results (p), or save them to:
			*
			* 0: specified location
			*
			* 1: CSV directory
			*
			* 2: Program directory
			*
			* noting if any dir is now unavailable rather than removing the option to ensure consistent numbering.
			*/
			[[nodiscard]] static auto promptForLocation(
				const std::string& CSVPathString,
				const std::string& programPathString
			) {
				auto entries = Transformers::Mapping::TransformerEntries<ExportLocation>{
				    {"0", ExportLocation::Specified},
					{"p", ExportLocation::Terminal},
				};

				std::string prompt = "Enter 0 to specify an output directory;\n\n";

				// This branch applies when both paths are either different or unavailable,
				// since we must have == for 2nd check to trigger.
				if (CSVPathString != programPathString || CSVPathString == directoryUnavailableLabel) {
					prompt += std::format(
						"OR Enter 1 to export results to same directory as data:\n{}\n\n"
						"OR Enter 2 to export results to same directory as program:\n{}\n\n",
						CSVPathString, programPathString
					);

					entries.emplace_back("1", ExportLocation::Data);
					entries.emplace_back("2", ExportLocation::Program);
				}
				else {
					prompt += std::format(
						"OR enter 1 to export results to same directory as data / program:\n{}\n\n",
						CSVPathString
					);

					entries.emplace_back("1", ExportLocation::Data);
				}

				prompt += "OR enter \"p\" to print results to terminal\n\n";
				prompt += "OR press ENTER to quit:";

				return Transformers::Mapping::mappingTransformer<ExportLocation>(
					prompt,
					std::move(entries),
					{.caseSensitive = false}
				);
			}
		}

		namespace Filename
		{
			struct FilenameGenerationError final : std::runtime_error
			{
				using std::runtime_error::runtime_error;
			};

			[[nodiscard]] static std::filesystem::path generateOutputFilename(const std::filesystem::path& dir) {
				if (std::error_code ec; !std::filesystem::is_directory(dir, ec) || ec) {
					throw FilenameGenerationError(std::format("Unable to access directory {}", dir.string()));
				}
				// Filename set in header.
				std::filesystem::path baseCandidate = dir / std::format("{}.csv", RESULTS_FILENAME);
				if (std::error_code ec; !std::filesystem::exists(baseCandidate, ec)) {
					return baseCandidate;
				}
				// Limit set in header.
				for (int i = 2; i <= RESULT_FILES_LIMIT; ++i) {
					std::filesystem::path numCandidate = dir / std::format("{}_{}.csv", RESULTS_FILENAME,i);
					if (std::error_code ec; !std::filesystem::exists(numCandidate, ec)) {
						return numCandidate;
					}
				}
				throw FilenameGenerationError(std::format("Too many result files exist"));
			}
		}
	}

	ExportDecision getExportDecision(const Domain::BondReturnData& tenorData) {
		std::filesystem::path filePath{};

		std::filesystem::path outputDirectory{};

		while (true) {
			// Get desired location:

			auto CSVPathOpt = Detail::Paths::getCSVPath(tenorData);
			auto programPathOpt = Detail::Paths::getProgramPath();

			auto CSVPathString = Detail::Paths::pathOrUnavailable(CSVPathOpt);
			auto programPathString = Detail::Paths::pathOrUnavailable(programPathOpt);

			auto locationPromptResult = Detail::LocationPrompt::promptForLocation(CSVPathString, programPathString);
			if (locationPromptResult.isEscape()) {
				return Decision::Quit{};
			}
			auto location = locationPromptResult.getValue();

			// Handle location choice:

			constexpr std::string_view dirUnavailableErrorMessage =
				"Directory unavailable (may have been renamed or deleted)";

			switch (location) {
				case Detail::ExportLocation::Specified: {
					const auto specifiedPathPromptResult = Transformers::promptTransformer(
						"Enter path to directory;\n"
						"OR press ENTER to see options again:",
						[](const std::string_view sv) -> Transformers::TransformerResult<std::filesystem::path> {
							if (sv.empty()) {
								return Transformers::Escape{};
							}
							std::filesystem::path result{};
							try {
								result = Helpers::Filesystem::expandUserPath(sv);
								Helpers::Filesystem::assertDirectoryValid(result);
							}
							catch (Helpers::Filesystem::DirectoryError &e) {
								return Transformers::Retry(std::format("Directory error: {}", e.what()));
							}
							return result;
						}
					);
					if (specifiedPathPromptResult.isEscape()) {
						continue;
					}
					outputDirectory = specifiedPathPromptResult.getValue();
					break;
				}

				case Detail::ExportLocation::Data:
					if (!CSVPathOpt) {
						Helpers::Printing::styledPrintln(Helpers::Printing::Styles::error, dirUnavailableErrorMessage);
						std::println();
						continue;
					}
					outputDirectory = std::filesystem::path{CSVPathString};
					break;

				case Detail::ExportLocation::Program:
					if (!programPathOpt) {
						Helpers::Printing::styledPrintln(Helpers::Printing::Styles::error, dirUnavailableErrorMessage);
						std::println();
						continue;
					}
					outputDirectory = std::filesystem::path{programPathString};
					break;

				case Detail::ExportLocation::Terminal:
					return Decision::Print{};
			}
			try {
				filePath = Detail::Filename::generateOutputFilename(outputDirectory);
			}
			catch (const Detail::Filename::FilenameGenerationError& e) {
				Helpers::Printing::styledPrintln(Helpers::Printing::Styles::error, "{}", e.what());
				std::println();
				continue;
			}

			break;
		}
		std::println();
		return Decision::Save{std::move(filePath)};
	}
}