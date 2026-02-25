#ifndef BSO_APP_CLI_PROMPTS_HPP
#define BSO_APP_CLI_PROMPTS_HPP

#include "transformers/Generic.hpp"

namespace Domain
{
	// Forward declaration, implemented in "include/app/domain/BondReturnData.hpp".
	class BondReturnData;
}

namespace Prompts
{
	/// Will warn the user if they request more than the specified number of results.
	constexpr int REQUEST_WARNING_NUM = 1'000'000;

	using DataPromptResult = Transformers::PromptResult<Domain::BondReturnData>;
	/// Prompts the user for the path to the bond data csv.
	[[nodiscard]] DataPromptResult getDataPrompt();

	using NumResultsPromptResult = Transformers::PromptResult<int>;
	/// Prompts the user for the number of results they'd like to calculate.
	[[nodiscard]] NumResultsPromptResult getNumResultsPrompt();
}

#endif //BSO_APP_CLI_PROMPTS_HPP