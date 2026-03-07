#ifndef BSO_TRANSFORMERS_MAPPING_HPP
#define BSO_TRANSFORMERS_MAPPING_HPP

#include "helpers/Quit.hpp"
#include "helpers/Strings.hpp"
#include "transformers/Generic.hpp"

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Transformers::Mapping
{
	/// Stores options for the mapping transformer, including which token should return an escape, whether or not
	/// tokens are case-sensitive (i.e. if "{x, value}" is specified, should we map "X" to value also).
	struct MappingOptions
	{
		std::string escapeToken{};
		bool caseSensitive = true;
		std::string errorMessage = "Invalid entry";
		std::string quitWord = std::string{Helpers::Quit::DEFAULT_QUIT_WORD};
	};

//----------------------------------------------------------------------------------------------------------------------

	/// The transformer should be specific via a list of {token, value} pairs, entering token to the prompt
	/// will then have the transformer return value.
	template <typename T>
	using TransformerEntries = std::vector<std::pair<std::string, T>>;

//----------------------------------------------------------------------------------------------------------------------

	namespace Detail
	{
		[[nodiscard]] inline std::string normaliseString(const std::string& s, const MappingOptions& options) {
			if (options.caseSensitive) {
				return s;
			}
			return Helpers::Strings::svToLowercase(s);
		}

		/// Ensures that there are no duplicate keys, and that no key matches the escape token,
		/// taking into account the specified case-sensitivity setting.
		template <typename T>
		[[nodiscard]] auto generateValueMap(const TransformerEntries<T>& entries, const MappingOptions& options) {
			const std::string normalisedEscapeToken = normaliseString(options.escapeToken, options);
			std::unordered_map<std::string, std::string> normalisedToOriginal;
			std::unordered_map<std::string, T> valueMap;
			for (const auto& [key, value] : entries) {
				std::string normalisedKey = normaliseString(key, options);
				if (normalisedKey == normalisedEscapeToken) {
					throw std::invalid_argument(
						std::format(
							"Escape token \"{}\" coincides with key \"{}\" (under current case-sensitivity settings)",
							options.escapeToken, key
						)
					);
				}
				auto [it, inserted] = normalisedToOriginal.emplace(normalisedKey, key);
				if (!inserted) {
					throw std::invalid_argument(
						std::format(
							"Key collision: \"{}\" conflicts with \"{}\" (under current case-sensitivity settings)",
							key, it->second
						)
					);
				}
				valueMap.emplace(normalisedKey, value);
			}
			return valueMap;
		}

//----------------------------------------------------------------------------------------------------------------------

		/// Creates a transformer from the list of {token, value} pairs, mapping each token to the specified value
		/// subject to case-sensitivity settings, handling the escape token and invalid input also.
		template <typename T>
		[[nodiscard]] auto makeMappingTransformer(TransformerEntries<T> entries, MappingOptions options = {}) {
			auto valueMap = Detail::generateValueMap(entries, options);
			// Capture by value and move to avoid copies:
			return [
				valueMap = std::move(valueMap),
				options = std::move(options)
			] (const std::string_view sv) -> TransformerResult<T> {
				const std::string normalisedInput = normaliseString(std::string{sv}, options);
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
				if (auto it = valueMap.find(normalisedInput); it != valueMap.end()) {
					return it->second;
				}
				return Retry(options.errorMessage);
			};
		}
	}

//----------------------------------------------------------------------------------------------------------------------

	/// Prompts the user with the specified message until they enter either a value specified in the
	/// TransformerEntries or the escape token, printing the error message before each reattempt.
	/// The return type in this case must be provided to the template.
	template <typename T>
	[[nodiscard]] PromptResult<T> mappingTransformer(
		const std::string_view prompt,
		TransformerEntries<T> entries,
		MappingOptions options
	) {
		auto transformer = Detail::makeMappingTransformer<T>(std::move(entries), std::move(options));
		return promptTransformer(prompt,std::move(transformer));
	}
}

#endif // BSO_TRANSFORMERS_MAPPING_HPP