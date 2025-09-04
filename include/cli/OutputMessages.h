#ifndef OUTPUT_MESSAGES_H
#define OUTPUT_MESSAGES_H

#include <exception>

/// Implemented in "src/types/BondReturnData.cpp".
class ExtremeList;

namespace OutputMessages
{
	/// Defines an error to be thrown if, for whatever reason, no viable combination of tenor choices can be found.
	struct NoSolutionsError final : std::exception {
		[[nodiscard]] const char* what() const noexcept override {
			return "no solutions found";
		}
	};

	/// Implemented in "src/cli/OutputMessages.cpp".
	void printExtremeResults(const ExtremeList& extremeList, int numSolutions);

	/// Implemented in "src/cli/OutputMessages.cpp".
	void printFileHelp();
}

#endif //OUTPUT_MESSAGES_H