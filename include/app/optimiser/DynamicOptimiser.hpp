#ifndef BSO_APP_OPTIMISER_DYNAMIC_OPTIMISER_HPP
#define BSO_APP_OPTIMISER_DYNAMIC_OPTIMISER_HPP

#include <vector>

namespace Domain
{
	// Forward declaration, implemented in "include/app/domain/InvestmentAction.hpp".
	class InvestmentAction;
	// Forward declaration, implemented in "include/app/domain/BondReturnData.hpp".
	class BondReturnData;
}

namespace DynamicOptimiser
{
	struct OptimalResults
	{
		std::vector<double> CRFs{}; // sorted CRFs
		std::vector<std::vector<Domain::InvestmentAction>> decisions{}; // reconstructed decision paths
	};

	/// Given BondReturnData, returns the requested number of optimal results (or as many as found if fewer),
	/// comprising the CRFs themselves and the path of InvestmentActions to achieve these.
	[[nodiscard]] OptimalResults getOptimalSequences(const Domain::BondReturnData& tenorData, int numResultsRequested);
}

#endif // BSO_APP_OPTIMISER_DYNAMIC_OPTIMISER_HPP