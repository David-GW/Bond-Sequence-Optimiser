#include "helpers/printing/Print.hpp"

#include "helpers/Output.hpp"

#include <print>
#include <string>

namespace Helpers::Printing
{
	void printRule() {
		std::println("{}", std::string(Output::getTerminalWidth(), '-'));
	}
}