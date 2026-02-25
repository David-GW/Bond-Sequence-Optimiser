#ifndef BSO_HELPERS_PRINTING_STYLED_PRINT_HPP
#define BSO_HELPERS_PRINTING_STYLED_PRINT_HPP

#include "rang/include/rang.hpp"

#include "helpers/Output.hpp"

#include <format>
#include <mutex>
#include <ostream>
#include <print>
#include <string>
#include <utility>

namespace Helpers::Printing
{
	/// Stores options for styled printing, text can be bold, italic, and/or underlined,
	/// the foreground and background colours can also be specified.
	struct Style
	{
		rang::fg fg = rang::fg::reset;
		rang::bg bg = rang::bg::reset;
		bool bold = false;
		bool italic = false;
		bool underline = false;

		friend std::ostream& operator<<(std::ostream& os, const Style& s) {
			os << s.fg << s.bg;
			if (s.bold) {
				os << rang::style::bold;
			}
			if (s.italic) {
				os << rang::style::italic;
			}
			if (s.underline) {
				os << rang::style::underline;
			}
			return os;
		}
	};

//----------------------------------------------------------------------------------------------------------------------

	namespace Styles
	{
		inline Style error{.fg = rang::fg::red};
	}

//----------------------------------------------------------------------------------------------------------------------

	namespace Detail
	{
		/// Used to ensure that calls to a styled print from multiple threads don't overwrite each other's
		/// styles mid-print.
		inline std::mutex osMutex;

		/// Ensures that rang is initialised with default settings once per program execution, and only if necessary.
		void rangConfigureOnce();
	}

//----------------------------------------------------------------------------------------------------------------------

	/// An interface to std::print, explicitly specifying the output stream,
	/// in which the output is styled as specified.
	template<class... Args>
	void styledPrint(std::ostream& os, const Style& style, std::format_string<Args...> fmt, Args&&... args) {
	    Detail::rangConfigureOnce();

		// Avoid threads conflicting:
	    std::lock_guard lock(Detail::osMutex);

	    os << style;
	    std::print(os, fmt, std::forward<Args>(args)...);
	    os << rang::style::reset;
	}

	/// An interface to std::print, defaulting to std::cout, in which the output is styled as specified.
	template<class... Args>
	void styledPrint(const Style& style, std::format_string<Args...> fmt, Args&&... args) {
	    styledPrint(std::cout, style, fmt, std::forward<Args>(args)...);
	}

//----------------------------------------------------------------------------------------------------------------------

	/// An equivalent to std::println, explicitly specifying the output stream,
	/// in which the output is styled as specified.
	template<class... Args>
	void styledPrintln(std::ostream& os, const Style& style, std::format_string<Args...> fmt, Args&&... args) {
		styledPrint(os, style, fmt, std::forward<Args>(args)...);
		os << '\n';
	}

	/// An equivalent to std::println, defaulting to std::cout, in which the output is styled as specified.
	template<class... Args>
	void styledPrintln(const Style& style, std::format_string<Args...> fmt, Args&&... args) {
		styledPrintln(std::cout, style, fmt, std::forward<Args>(args)...);
	}

//----------------------------------------------------------------------------------------------------------------------

	/// An equivalent to std::print, explicitly specifying the output stream,
	/// in which the output is styled as specified and wrapped to the terminal width.
	template<class... Args>
	void styledWrappedPrint(std::ostream& os, const Style& style, std::format_string<Args...> fmt, Args&&... args) {
		styledPrint(os, style, "{}", Output::wrapText(fmt, std::forward<Args>(args)...));
	}

	/// An equivalent to std::print, defaulting to std::cout,
	/// in which the output is styled as specified and wrapped to the terminal width.
	template<class... Args>
	void styledWrappedPrint(const Style& style, std::format_string<Args...> fmt, Args&&... args) {
		styledWrappedPrint(std::cout, style, fmt, std::forward<Args>(args)...);
	}

//----------------------------------------------------------------------------------------------------------------------

	/// An equivalent to std::println, explicitly specifying the output stream,
	/// in which the output is styled as specified and wrapped to the terminal width.
	template<class... Args>
	void styledWrappedPrintln(std::ostream& os, const Style& style, std::format_string<Args...> fmt, Args&&... args) {
		styledPrintln(os, style, "{}", Output::wrapText(fmt, std::forward<Args>(args)...));
	}

	/// An equivalent to std::println, defaulting to std::cout,
	/// in which the output is styled as specified and wrapped to the terminal width.
	template<class... Args>
	void styledWrappedPrintln(const Style& style, std::format_string<Args...> fmt, Args&&... args) {
		styledWrappedPrintln(std::cout, style, fmt, std::forward<Args>(args)...);
	}
}

#endif // BSO_HELPERS_PRINTING_STYLED_PRINT_HPP