#ifndef BSO_HELPERS_STRINGS_HPP
#define BSO_HELPERS_STRINGS_HPP

#include <format>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

/// Contains functions of general utility to the rest of the codebase.
namespace Helpers::Strings
{
	/// Returns a string from a string_view by converting all characters to lowercase,
	/// safely in case of non-ASCII characters.
	[[nodiscard]] std::string svToLowercase(std::string_view sv);

	/// Compares two string_views disregarding case, safely in case of non-ASCII characters.
	[[nodiscard]] bool svCaseInsensitiveCompare(std::string_view sv1, std::string_view sv2);

	/// Performs an in-place removal of any whitespace at the beginning or end of a string_view.
	void svTrimWhitespaceInPlace(std::string_view& sv) noexcept;

	[[nodiscard]] bool svIsPositiveInt(std::string_view sv) noexcept;

//----------------------------------------------------------------------------------------------------------------------

	template<typename Int>
	requires std::is_integral_v<Int>
	[[nodiscard]] std::string formatIntWithSeparator(
		const Int n,
		const std::string& separator = ",",
		const int blockSize = 3
	) {
		std::string s = std::to_string(n);
		int pos = static_cast<int>(s.length()) - blockSize;
		const bool negative = std::is_signed_v<Int> && n < 0;
		const int end = negative ? 1 : 0;
		while (pos > end) {
			s.insert(pos, separator);
			pos -= blockSize;
		}
		return s;
	}

//----------------------------------------------------------------------------------------------------------------------

	/// Returns a string of elements of a formattable range separated by the specified delimiter,
	/// which is ", " by default, with each element formatted as specified, which is "{}" by default.
	template <std::ranges::input_range R>
	requires std::formattable<std::remove_cvref_t<std::ranges::range_reference_t<R>>, char>
	[[nodiscard]] std::string joinFormatted(
		const R& range,
		const std::string_view delimiter = ", ",
		// Note: formatSpec is runtime data, so validity cannot be checked at compile time.
		// To have joinFormatted be checked at compile, remove this argument and use:
		// std::format_to(std::back_inserter(result), "{}", *it);
		const std::string_view formatSpec = "{}"
	) {
		std::string result{};

		auto it = std::ranges::begin(range);
		const auto end = std::ranges::end(range);

		if (it == end) {
			return result;
		}

		std::vformat_to(std::back_inserter(result), formatSpec, std::make_format_args(*it));
		for (++it; it != end; ++it) {
			result.append(delimiter);
			std::vformat_to(std::back_inserter(result), formatSpec, std::make_format_args(*it));
		}

		return result;
	}
}

#endif // BSO_HELPERS_STRINGS_HPP