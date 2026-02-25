#ifndef BSO_HELPERS_QUIT_HPP
#define BSO_HELPERS_QUIT_HPP

#include <string_view>

namespace Helpers::Quit
{
	/// The default word to be used when confirming quit,
	/// since sometimes "abort" or another word may want to be used.
	inline constexpr std::string_view DEFAULT_QUIT_WORD = "quit";

	/// Asks the user to press ENTER in order to approve quitting.
	[[nodiscard]] bool confirmQuit(std::string_view quitWord = DEFAULT_QUIT_WORD);
}

#endif // BSO_HELPERS_QUIT_HPP