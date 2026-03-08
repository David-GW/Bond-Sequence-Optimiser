#ifndef BSO_TRANSFORMERS_OPTIONS_HPP
#define BSO_TRANSFORMERS_OPTIONS_HPP

#include "helpers/Strings.hpp"

#include <concepts>
#include <string>
#include <string_view>

namespace Transformers
{
	/// Used to define functions for pre-built transformers with options to specify the escape token
	/// and case sensitivity.
	template <typename T>
	concept HasEscapeOptions = requires(const T& t) {
		{ t.escapeToken } -> Helpers::Strings::StringLike;
		{ t.caseSensitive } -> std::convertible_to<bool>;
	};

	/// Returns a string with the input converted to lowercase if the transformer is case-sensitive.
	template <HasEscapeOptions Options>
	[[nodiscard]] std::string normaliseString(const std::string_view sv, const Options& options) {
		if (options.caseSensitive) {
			return std::string{sv};
		}
		return Helpers::Strings::svToLowercase(sv);
	}

	/// Checks if the input is a request to escape under the transformer's options.
	template <HasEscapeOptions Options>
	[[nodiscard]] bool checkEscape(const std::string_view sv, const Options& options) {
		if (options.caseSensitive) {
			return sv == options.escapeToken;
		}
		return Helpers::Strings::svCaseInsensitiveCompare(sv, options.escapeToken);
	}
}
#endif // BSO_TRANSFORMERS_OPTIONS_HPP