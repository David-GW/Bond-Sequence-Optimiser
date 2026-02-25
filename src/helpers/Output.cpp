#include "helpers/Output.hpp"

#if defined(_WIN32) || defined(_WIN64)
	#define IS_WINDOWS 1
#else
	#define IS_WINDOWS 0
#endif

#if IS_WINDOWS
	#include <windows.h>
#else
	#include <unistd.h>
	#include <sys/ioctl.h>
#endif

namespace Helpers::Output
{
	int getTerminalWidth(const int fallback) noexcept {
		#if IS_WINDOWS
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
				if (const int result = csbi.srWindow.Right - csbi.srWindow.Left + 1; result > 0) {
					return result;
				}
			}
		#else
			winsize w{};
			if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
				if (const int result = w.ws_col; result > 0) {
					return result;
				}
			}
		#endif
		return fallback;
	}
}