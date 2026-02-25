#include "helpers/Quit.hpp"

#include <iostream>
#include <print>
#include <string>
#include <string_view>

namespace Helpers::Quit
{
	bool confirmQuit(const std::string_view quitWord) {
		std::println("Press ENTER to confirm {};", quitWord);
		std::println("OR enter anything else to go back:", quitWord);
		std::string input{};
		return !std::getline(std::cin, input) || input.empty();
	}
}