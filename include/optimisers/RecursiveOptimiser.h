#ifndef RECURSIVE_OPTIMISER_H
#define RECURSIVE_OPTIMISER_H

#include <tuple>

// Implemented in "include/types/BondReturnData.h".
class BondReturnData;

// Implemented in "include/types/RankingTypes.h".
class ExtremeList;

/**
* Contains the recursive algorithm and its interface to calculate the specified number of top and bottom CRFs
* with their corresponding buying strategies, as well as the total number of distinct valid strategies.
*/
namespace RecursiveOptimiser
{
	// Implemented in "src/optimisers/RecursiveOptimiser.cpp".
	[[nodiscard]] std::tuple<ExtremeList, ExtremeList, int>
	topBotCRFs(const BondReturnData& tenorData, int numTopChoices, int numBotChoices);
}

#endif // RECURSIVE_OPTIMISER_H