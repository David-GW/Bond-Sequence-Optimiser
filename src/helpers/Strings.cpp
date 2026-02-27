#include "helpers/Strings.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <ranges>
#include <string>
#include <string_view>

/// Contains functions of general utility to the rest of the codebase.
namespace Helpers::Strings
{
	namespace Detail
	{
		[[nodiscard]] inline char charToLowercaseSafe(const char c) noexcept {
			// Explicit casting avoids problems such as non-ASCII characters being negatively signed.
			const auto uc = static_cast<unsigned char>(c);
			return static_cast<char>(std::tolower(uc));
		}
	}

	std::string svToLowercase(const std::string_view sv) {
		std::string result{sv};
		std::ranges::transform(result,result.begin(), Detail::charToLowercaseSafe);
		return result;
	}

	bool svCaseInsensitiveCompare(const std::string_view sv1, const std::string_view sv2) {
		return std::ranges::equal(
			sv1, sv2, {}, Detail::charToLowercaseSafe, Detail::charToLowercaseSafe
		);
	}

	void svTrimWhitespaceInPlace(std::string_view& sv) noexcept {
		// Explicit casting avoids problems such as non-ASCII characters being negatively signed.

		// Trim leading whitespace:
		while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
			sv.remove_prefix(1);
		}
		// Trim trailing whitespace:
		while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
			sv.remove_suffix(1);
		}
	}

	bool svIsPositiveInt(const std::string_view sv) noexcept {
		int value{};
		if (
			auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
			ec != std::errc{} || ptr != sv.data() + sv.size()
		) {
			return false; // invalid string or integer overflows
		}
		return value > 0;
	}
}