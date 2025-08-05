#include "helpers/GeneralHelpers.h"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string>
#include <string_view>

/// Contains functions of general utility to the rest of the codebase.
namespace GeneralHelpers
{
	/// Determines whether every character of a string_view is a digit 0-9.
	bool svAllDigits(const std::string_view sv) {
		return std::ranges::all_of(sv, [](const char c) {
			// Explicit casting avoids problems such as non-ASCII characters being promoted to ints.
			return std::isdigit(static_cast<unsigned char>(c));
		});
	}

	/// Performs an in-place removal of any spaces at the beginning or end of a string_view.
	void svTrimWhitespace(std::string_view& sv) {
		// Explicit casting avoids problems such as non-ASCII characters being promoted to ints.

		// Trim leading whitespace.
		while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
			sv.remove_prefix(1);
		}

		// Trim trailing whitespace.
		while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
			sv.remove_suffix(1);
		}
	}

	/// Takes a string_view and returns the corresponding string in lowercase.
	std::string svToLowercase(const std::string_view sv) {
		std::string result(sv);
		std::ranges::transform(result, result.begin(), [](const unsigned char c) {
			return std::tolower(c);
		});
		return result;
	}
}