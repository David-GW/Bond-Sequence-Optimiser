#ifndef BSO_TRANSFORMERS_NUMERIC_HPP
#define BSO_TRANSFORMERS_NUMERIC_HPP

#include "helpers/Quit.hpp"
#include "helpers/Strings.hpp"
#include "transformers/Generic.hpp"

#include <charconv>
#include <format>
#include <string>
#include <string_view>

namespace Transformers::Numeric
{
	/// Stores options for the numeric transformer, including which token should return an escape,and whether or not
	/// this token is case-sensitive.
	struct NumericOptions
	{
		std::string escapeToken{};
		bool caseSensitive = true;
		std::string errorMessage = "Invalid entry";
		std::string quitWord = std::string{Helpers::Quit::DEFAULT_QUIT_WORD};
		std::string mustBePositiveMessage = "Entry must be a positive integer";
		std::string tooLargeMessage = "Entry too large";
	};

//----------------------------------------------------------------------------------------------------------------------

	namespace Detail
	{
		/// Creates a transformer expecting a positive int, returning that if it receives one, handling the
		/// escape token and invalid input also.
		[[nodiscard]] inline auto makePositiveIntTransformer(NumericOptions options = {}) {
			return [options = std::move(options)] (const std::string_view sv) -> TransformerResult<int> {
				bool escapeRequested = false;
				if (options.caseSensitive) {
					escapeRequested = sv == options.escapeToken;
				}
				else {
					escapeRequested = Helpers::Strings::svCaseInsensitiveCompare(sv, options.escapeToken);
				}
				if (escapeRequested) {
					if (options.quitWord.empty() || Helpers::Quit::confirmQuit(options.quitWord)) {
						return Escape{};
					}
					return Retry{};
				}

				int result{};
				// Commented code below to be used when MSVC updates support:
				// auto [ptr, ec] = std::from_chars(sv.begin(), sv.end(), result);
				auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);

				if (ec == std::errc::result_out_of_range) {
					if (!sv.empty() && sv.front() == '-') {
						return Retry(options.mustBePositiveMessage);
					}
					return Retry(options.tooLargeMessage);
				}
				// Commented code below to be used when MSVC updates support:
				// if (ec == std::errc::invalid_argument || ptr != sv.end()) {
				if (ec == std::errc::invalid_argument || ptr != sv.data() + sv.size()) {
					return Retry(options.errorMessage);
				}
				if (result <= 0) {
					return Retry(options.mustBePositiveMessage);
				}
				return result;
			};
		}
	}

//----------------------------------------------------------------------------------------------------------------------

	/// Prompts the user with the specified message until they enter either a positive integer or the escape token,
	/// printing the error message before each reattempt.
	[[nodiscard]] inline PromptResult<int> positiveIntTransformer(
		const std::string_view prompt,
		NumericOptions options = {}
	) {
		// Ensures that the escape token isn't a valid integer.
		if (Helpers::Strings::svIsPositiveInt(options.escapeToken)) {
			throw std::invalid_argument(
				std::format("Escape token \"{}\" collides with valid numeric input", options.escapeToken)
			);
		}
		auto transformer = Detail::makePositiveIntTransformer(options);
		return promptTransformer(prompt,std::move(transformer));
	}
}

#endif // BSO_TRANSFORMERS_NUMERIC_HPP