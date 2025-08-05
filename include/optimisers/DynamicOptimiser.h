#ifndef DYNAMIC_OPTIMISER_H
#define DYNAMIC_OPTIMISER_H

#include <utility>
#include <vector>

// Implemented in "src/types/BondReturnData.h".
class BondReturnData;

// Implemented in "src/types/InvestmentAction.h".
class InvestmentAction;

/// Contains the dynamic programming algorithm to calculate the optimal CRF and buying strategy.
namespace DynamicOptimiser
{
	// Implemented in "src/optimisers/DynamicOptimiser".
	[[nodiscard]] std::pair<double, std::vector<InvestmentAction>> optimiseCRF(const BondReturnData& tenorData);
}

#endif // DYNAMIC_OPTIMISER_H