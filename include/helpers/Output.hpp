#ifndef BSO_HELPERS_OUTPUT_HPP
#define BSO_HELPERS_OUTPUT_HPP

#include "TextFlow/include/TextFlow.hpp"

#include <format>
#include <iostream>
#include <string>
#include <utility>

namespace Helpers::Output
{
	[[nodiscard]] int getTerminalWidth(int fallback = 80) noexcept;

//----------------------------------------------------------------------------------------------------------------------

	/// Returns the given string with line breaks inserted to wrap to the terminal width.
	template<class... Args>
	std::string wrapText(std::format_string<Args...> fmt, Args&&... args) {
		const auto formatted = std::format(fmt, std::forward<Args>(args)...);
		const auto output = TextFlow::Column(formatted).width(getTerminalWidth());
		return output.toString();
	}
}

#endif // BSO_HELPERS_OUTPUT_HPP