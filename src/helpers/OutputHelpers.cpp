#include "helpers/OutputHelpers.h"

// Loads the appropriate libraries for getting terminal width based on OS.
#ifdef _WIN32
	#include <windows.h>
#else // !(_WIN32)
	#include <sys/ioctl.h>
	#include <unistd.h>
#endif // _WIN32

#include <cctype>
#include <cstddef>
#include <print>
#include <sstream>
#include <string>

/// Contains functions which mainly help to output to the terminal.
namespace OutputHelpers
{
	/**
	* Uses separate logic for Windows and MacOS/Linux systems to get the width of the current terminal window in characters,
	* defaulting to 80 if this fails.
	*/
	int getTerminalWidth(const int fallback) {
		#ifdef _WIN32
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
				if (const int result = csbi.srWindow.Right - csbi.srWindow.Left + 1; result > 0) {
					return result;
				}
			}
		#else // !(_WIN32)
			winsize w{};
			if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
				if (const int result = w.ws_col; result > 0) {
					return result;
				}
			}
		#endif // _WIN32
		return fallback; // fallback if detection fails
	}

	/// Prints a horizontal rule of --- the width of the terminal, or the default value of getTerminalWidth if detection fails.
	void printRule() {
		std::println("{}", std::string(getTerminalWidth(), '-'));
	}

	/**
	* Inserts newlines into a string at the last word in each line which would cause the string to exceed the specified width.
	*
	* IMPORTANT: this function strips any whitespace in the original string, so manual new lines are removed for example.
	*/
	std::string wrapText(const std::string& text, std::size_t width) {
		std::istringstream words(text);
		std::ostringstream wrapped;

		std::string word;
		std::size_t line_length = 0;

		while (words >> word) {
			// Adds a newline after the last word BEFORE the terminal width would be exceeded.
			if (line_length + word.size() + 1 > width) {
				wrapped << '\n';
				line_length = 0;
			}
			if (line_length > 0) {
				wrapped << ' ';
				++line_length;
			}
			wrapped << word;
			line_length += word.size();
		}
		return wrapped.str();
	}
}