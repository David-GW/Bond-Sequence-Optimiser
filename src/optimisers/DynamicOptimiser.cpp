#include "optimisers/DynamicOptimiser.h"

#include "types/BondReturnData.h"
#include "types/InvestmentAction.h"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <utility>
#include <vector>

/// Contains the dynamic programming algorithm to calculate the optimal CRF and buying strategy.
namespace DynamicOptimiser
{
	/**
	* Takes the data on bond returns, and returns { CRF, choices }, where CRF is the optimal cumulative return factor,
	* and choices stores the buying strategy necessary to achieve this.
	*/
	std::pair<double, std::vector<InvestmentAction>> optimiseCRF(const BondReturnData& tenorData)
	{
		// CALCULATE OPTIMAL CRF ---------------------------------------------------------------------------------------

		const std::size_t numMonths = tenorData.numMonths();
		const std::size_t numTenors = tenorData.tenors().size();

		// monthlyBestCRF[m] will store the optimal cumulative return factor at month  m,
		// initialised to { 1.0, ..., 1.0 } for months 0 to numMonths, since we can always achieve a CRF of 1 by waiting,
		// i.e. not making any purchases.
		std::vector monthlyBestCRF(numMonths + 1, 1.0);

		// monthlyBestTenor[m] will store the optimal tenor to reach month m, or whether to wait from the previous month
		// (represented by 0), initialised to { 0, ..., 0 } for months 0 to numMonths.
		std::vector monthlyBestTenor(numMonths + 1, 0);

		// For a given month, we work out the best tenor to reach it by calculating potential CRFs from each tenor
		for (std::size_t currentMonth = 1; currentMonth <= numMonths; ++currentMonth) {
			// As a minimum, we know we can stay at the same CRF as last month just by waiting.
			double currentMonthBestCRF = monthlyBestCRF[currentMonth - 1];
			// As noted, we represent waiting a month with a tenor of length 0.
			int bestTenorToReachMonth = 0;

			for (std::size_t i = 0; i < numTenors; ++i) {
				const int currentTenor = tenorData.tenors()[i];

				// Stop if the tenor length exceeds the current month, since then there is no month a bond of that tenor
				// can purchased for it to be sold in the current month.
				// Since the tenors are in ascending order, we can break not just continue.
				if (currentMonth < static_cast<size_t>(currentTenor)) {
					break;
				}

				// Get the return factor for the appropriate tenor and month, calculate the resulting CRF,
				// and check if this is an improvement on the current best.
				const double returnFactor = 1.0 +
					tenorData(static_cast<int>(i), static_cast<int>(currentMonth) - currentTenor);

				if (const double currentCRF = monthlyBestCRF[currentMonth - currentTenor] * returnFactor;
					currentCRF > currentMonthBestCRF)
				{
					currentMonthBestCRF = currentCRF;
					bestTenorToReachMonth = currentTenor;
				}
			}
			monthlyBestCRF[currentMonth] = currentMonthBestCRF;
			// This will still be 0 if waiting was the best option.
			monthlyBestTenor[currentMonth] = bestTenorToReachMonth;
		}

		// CONSTRUCT LIST OF OPTIMAL CHOICES ---------------------------------------------------------------------------

		std::vector<InvestmentAction> optimalChoices;

		// We need to start from the final month to reconstruct the path of optimal choices.
		int currentMonth = static_cast<int>(numMonths);

		// This keeps track of how long, if at all, we have been waiting, so rather than returning x 1-month waits,
		// we can return a single x-month wait.
		int waitStreak = 0;

		// To construct the best path, we work backwards until reaching month 0.
		while (currentMonth > 0) {
			int bestTenorToReachMonth = monthlyBestTenor[currentMonth];

			if (bestTenorToReachMonth == 0) {
				++waitStreak;
				--currentMonth;
				continue;
			}
			// If this condition is true, we have found the month a waiting streak began, so must add it to the path
			// and reset the waiting streak length counter.
			if (waitStreak > 0) {
				optimalChoices.emplace_back(
					InvestmentAction::Action::Wait,
					currentMonth - waitStreak,
					waitStreak
				);
				waitStreak = 0;
			}

			// We now know we are not waiting and have dealt with any prior wait streak, so can add the best tenor to the path
			// and continue from the month that was purchased.
			optimalChoices.emplace_back(
				InvestmentAction::Action::Buy,
				currentMonth - bestTenorToReachMonth,
				bestTenorToReachMonth
			);
			currentMonth -= bestTenorToReachMonth;
		}
		// Here we deal with the case that we have reached month 0 with an active waiting streak.
		if (waitStreak > 0) {
			optimalChoices.emplace_back(
				InvestmentAction::Action::Wait,
				0,
				waitStreak
			);
		}

		// Since we have (necessarily) constructed the path backwards, we reverse it for ease of use later.
		std::ranges::reverse(optimalChoices);

		return { monthlyBestCRF.back(), optimalChoices };
	}
}