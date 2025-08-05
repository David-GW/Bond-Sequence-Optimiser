#ifndef OUTPUT_HELPERS_H
#define OUTPUT_HELPERS_H

#include <cstddef>
#include <format>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>

/// Contains functions which mainly help to output to the terminal.
namespace OutputHelpers
{
	/// Replicates std::println, but only prints if non-empty, avoiding an empty string still printing a new line.
	template<typename... Args>
	void printIfNotEmpty(std::ostream& os, const std::string_view sv, Args&&... args) {
		if (sv.empty()) {
			return;
		}
		if (const std::string formatted = std::vformat(sv, std::make_format_args(args...)); !formatted.empty()) {
			std::format_to(std::ostream_iterator<char>(os), "{}\n", formatted);
		}
	}

	/// Replicates std::println(std::cout, ...), but only prints if non-empty, avoiding an empty string still printing a new line.
	template<typename... Args>
	void printIfNotEmpty(const std::string_view sv, Args&&... args) {
		printIfNotEmpty(std::cout, sv, std::forward<Args>(args)...);
	}

	// Implemented in "src/helpers/OutputHelpers.cpp".
	[[nodiscard]] int getTerminalWidth(int fallback = 80); // Implemented in "src/helpers/OutputHelpers.cpp"

	// Implemented in "src/helpers/OutputHelpers.cpp".
	void printRule();

	// Implemented in "src/helpers/OutputHelpers.cpp".
	[[nodiscard]] std::string wrapText(const std::string& text, std::size_t width);

	/**
	* Replicates std::println, but wraps text according to terminal width (defaulting to 80 characters).
	*
	* IMPORTANT: this function strips any whitespace in the original string, so manual new lines are removed for example.
	*/
	template<typename... Args>
	void wrappedPrint(std::ostream& os, const std::string_view sv, Args&&... args) {
		const std::string formatted = wrapText(std::vformat(sv, std::make_format_args(args...)), getTerminalWidth());
		std::format_to(std::ostream_iterator<char>(os), "{}\n", formatted);
	}

	/**
	* Replicates std::println(std::cout, ...), but wraps text according to terminal width (defaulting to 80 characters).
	*
	* IMPORTANT: this function strips any whitespace in the original string, so manual new lines are removed for example.
	*/
	template<typename... Args>
	void wrappedPrint(const std::string_view sv, Args&&... args) {
		wrappedPrint(std::cout, sv, std::forward<Args>(args)...);
	}
}

#endif // OUTPUT_HELPERS_H