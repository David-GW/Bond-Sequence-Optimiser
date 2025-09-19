#include "optimisers/DynamicOptimiser.h"

#include "types/BondReturnData.h"
#include "types/InvestmentAction.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

/// Contains the dynamic programming algorithm to calculate the optimal CRF and buying strategy.
namespace DynamicOptimiser
{
	/**
	* Takes the data on bond returns, and returns {CRF, choices}, where CRF is the optimal cumulative return factor,
	* and choices stores the buying strategy necessary to achieve this.
	*/
	std::pair<double, std::vector<InvestmentAction>> optimiseCRF(const BondReturnData& tenorData)
	{
		// CALCULATE OPTIMAL CRF ---------------------------------------------------------------------------------------

		const std::size_t numMonths = tenorData.numMonths();
		const std::size_t numTenors = tenorData.tenors().size();

		// monthlyBestCRF[m] will store the optimal cumulative return factor at month  m,
		// initialised to { 1, –∞, ..., –∞, –∞ } for months 0 to numMonths, since ANY CRF needs to beat the default,
		// even if negative for example, and so must be compared to –∞.
		std::vector monthlyBestCRF(numMonths + 1, -std::numeric_limits<double>::infinity());
		monthlyBestCRF[0] = 1.0;

		// monthlyBestTenor[m] will store the optimal tenor to reach month m (or -1 if none),
		// initialised to { -1, ..., -1 } for months 0 to numMonths.
		// Note that monthlyBestTenor[0] will always be -1, since there is no way to reach month 0.
		std::vector monthlyBestTenor(numMonths + 1, -1);

		// For a given month, we work out the best tenor to reach it by calculating potential CRFs from each tenor
		for (std::size_t currentMonth = 1; currentMonth <= numMonths; ++currentMonth) {
			double currentMonthBestCRF = -std::numeric_limits<double>::infinity();
			int bestTenorToReachMonth = -1;

			for (std::size_t i = 0; i < numTenors; ++i) {
				const int currentTenor = tenorData.tenors()[i];

				// Stop if the tenor length exceeds the current month, since then there is no month a bond of that tenor
				// can purchased for it to be sold in the current month.
				// Since the tenors are in ascending order, we can break not just continue.
				if (currentMonth < static_cast<size_t>(currentTenor)) break;

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

			// We need to check whether to buy or wait (i.e. make no purchase in the current month). This might happen if
			// delaying allows the purchase of a future bond resulting in a chain of higher CRFs than purchasing
			// in the current month would allow.
			if (monthlyBestCRF[currentMonth - 1] > currentMonthBestCRF) {
				currentMonthBestCRF = monthlyBestCRF[currentMonth - 1];
				bestTenorToReachMonth = 0;
			}
			monthlyBestCRF[currentMonth] = currentMonthBestCRF;
			// This will still be -1 if no path of tenors can reach the current month, or 0 if waiting was the best option.
			monthlyBestTenor[currentMonth] = bestTenorToReachMonth;
		}

		// Find the month with the highest CRF (this may not necessarily be the last depending on the tenors vs total months).
		int bestFinalMonth = 0;
		// As in the initialisation of monthlyBestCRF[m], we use –∞ since ANY CRF must beat this initial value.
		double bestCRF = -std::numeric_limits<double>::infinity();

		for (std::size_t currentMonth = 0; currentMonth <= numMonths; ++currentMonth) {
			if (monthlyBestCRF[currentMonth] > bestCRF) {
				bestCRF = monthlyBestCRF[currentMonth];
				bestFinalMonth = static_cast<int>(currentMonth);
			}
		}

		// CONSTRUCT LIST OF OPTIMAL CHOICES ---------------------------------------------------------------------------

		std::vector<InvestmentAction> optimalChoices;

		// We need to start from the month will the best CRF.
		int currentMonth = bestFinalMonth;

		// This keeps track of how long, if at all, we have been waiting, so rather than returning x 1-month waits,
		// we can return a single x-month wait.
		int waitStreak = 0;

		// To construct out the best path, we work backwards until reaching month 0.
		while (currentMonth > 0) {
			int bestTenorToReachMonth = monthlyBestTenor[currentMonth];
			if (bestTenorToReachMonth == -1) {
				// This should never happen, but is included to avoid the potential for an infinite loop.
				throw std::runtime_error("Error computing tenor choices");
			}
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

		return { bestCRF, optimalChoices };
	}
}