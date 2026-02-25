#ifndef BSO_HELPERS_PRINTING_PRINT_HPP
#define BSO_HELPERS_PRINTING_PRINT_HPP

#include "helpers/Output.hpp"

#include <format>
#include <iostream>
#include <print>
#include <utility>

namespace Helpers::Printing
{
	/// Prints a horizontal rule of --- the width of the terminal,
	/// or the default value of getTerminalWidth if detection fails.
	void printRule();

//----------------------------------------------------------------------------------------------------------------------

	/// An interface to std::print, explicitly specifying the output stream,
	/// in which the output is wrapped to the terminal width.
	template<class... Args>
	void wrappedPrint(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
		std::print(os, "{}", Output::wrapText(fmt, std::forward<Args>(args)...));
	}

	/// An interface to std::print, defaulting to std::cout, in which the output is wrapped to the terminal width.
	template<class... Args>
	void wrappedPrint(std::format_string<Args...> fmt, Args&&... args) {
		wrappedPrint(std::cout, fmt, std::forward<Args>(args)...);
	}

	/// An interface to std::println, explicitly specifying the output stream,
	/// in which the output is wrapped to the terminal width.
	template<class... Args>
	void wrappedPrintln(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
		std::println(os, "{}", Output::wrapText(fmt, std::forward<Args>(args)...));
	}

	/// An interface to std::println, defaulting to std::cout, in which the output is wrapped to the terminal width.
	template<class... Args>
	void wrappedPrintln(std::format_string<Args...> fmt, Args&&... args) {
		wrappedPrintln(std::cout, fmt, std::forward<Args>(args)...);
	}
}
#endif //BSO_HELPERS_PRINTING_PRINT_HPP