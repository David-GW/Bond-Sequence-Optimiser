#ifndef GENERAL_HELPERS_H
#define GENERAL_HELPERS_H

#include <concepts>
#include <format>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>

/// Contains functions of general utility to the rest of the codebase.
namespace GeneralHelpers
{
	// Implemented in "src/helpers/GeneralHelpers.cpp"
	[[nodiscard]] bool svAllDigits(std::string_view sv);

	// Implemented in "src/helpers/GeneralHelpers.cpp"
	void svTrimWhitespace(std::string_view& sv);

	// Implemented in "src/helpers/GeneralHelpers.cpp"
	[[nodiscard]] std::string svToLowercase(std::string_view sv);

	/// Ensures that T can be formatted by std::format.
	template <typename T>
	concept Formattable = std::formattable<T, char>;

	/**
	* Ensures that R is a range which can be iterated from begin() to end() once using input iterators,
	* and that the elements of R can be formatted by std::format.
	*/
	template <typename R>
	concept FormattableRange = std::ranges::input_range<R> && Formattable<std::ranges::range_value_t<R>>;

	/**
	* Returns a string of formatted elements of the formattable range separated by the specified delimiter,
	* which is ", " by default.
	*/
	template <FormattableRange Range>
	[[nodiscard]] std::string joinFormatted(const Range& range, const std::string_view delimiter = ", ") {
		std::ostringstream os;
		auto it = range.begin();
		if (it != range.end()) {
			os << std::format("{}", *it);
			++it;
		}
		while (it != range.end()) {
			os << delimiter << std::format("{}", *it);
			++it;
		}
		return os.str();
	}
}

#endif // GENERAL_HELPERS_H