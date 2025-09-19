#include "optimisers/RecursiveOptimiser.h"

#include "types/BondReturnData.h"
#include "types/RankingTypes.h"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

/**
* Contains the recursive algorithm and its interface to calculate the specified number of top and bottom CRFs
* with their corresponding buying strategies, as well as the total number of distinct valid strategies.
*/
namespace RecursiveOptimiser
{
	/// Contains helper functions used only in the RecursiveOptimiser namespace.
	namespace Detail
	{
		/**
		* Takes an ExtremeList, a candidate CRF, and its corresponding path. If, after performing the ranking
		* comparison, the candidate CRF should belong to the list, it is then inserted along with its path
		* as a CRFAndChoices at the appropriate position.
		*/
		void compareAndInsert(
			ExtremeList& extremeList,
			const double candidateCRF,
			const std::vector<int>& candidatePath
			) {
			auto& elemList = extremeList.rankedList();
			const bool isTop = extremeList.rankingType().kind == TopOrBottom::Kind::Top;

			const auto it = std::ranges::find_if(elemList, [&](const auto& e) {
				if (isTop) {
					return candidateCRF > e.CRF();
				}
				return candidateCRF < e.CRF();
			});
			if (it != elemList.end()) {
				// If the candidate CRF should belong to the list, insert it and its path as a CRFAndChoices
				// at the appropriate position, and shift any further elements down.
				std::rotate(it, elemList.end() - 1, elemList.end());
				*it = CRFAndChoices(candidateCRF, candidatePath);
			}
		}
	}

	/// Performs the DFS recursive step for topBotCRFs.
	void recurseCRFs(
		const BondReturnData& tenorData,
		const size_t currentMonth,
		const double currentCRF,
		std::vector<int>& currentPath,
		ExtremeList& topChoices,
		ExtremeList& botChoices,
		int& numSolutions
		) {
		const auto& tenors = tenorData.tenors();
		const std::size_t numTenors = tenors.size();
		const int shortestTenor = tenors.front();

		for (size_t i = 0; i < numTenors; ++i) {
			const int currentTenor = tenors[i];
			// Calculate when a bond of the current tenor bought in the current month would mature.
			const size_t newMonth = currentMonth + static_cast<size_t>(currentTenor);
			// Stop if this exceeds the total number of months provided, we must sell before the end of this period.
			// Since the tenors are in ascending order, we can break not just continue.
			if (newMonth > tenorData.numMonths()) {
				break;
			}

			// Add the current tenor to a path of tenor choices.
			currentPath.push_back(currentTenor);

			// Get the return factor for the current tenor and month, and calculate the resulting CRF.
			const double returnFactor = 1.0 + tenorData(static_cast<int>(i), static_cast<int>(currentMonth));
			const double newCRF = currentCRF * returnFactor;

			// If we cannot go any further, check if the resulting CRF (and its corresponding path)
			// should be added to the top/bottom result lists.
			if (newMonth + static_cast<std::size_t>(shortestTenor) > tenorData.numMonths()) {
				++numSolutions;
				Detail::compareAndInsert(topChoices, newCRF, currentPath);
				Detail::compareAndInsert(botChoices, newCRF, currentPath);
			}
			// Otherwise continue with the recursion, adding the current tenor to the list of choices and advancing that many months.
			else {
				recurseCRFs(tenorData, newMonth, newCRF, currentPath, topChoices, botChoices, numSolutions);
			}
			// Remove the current tenor from the path so it can be replaced with the next (if any).
			currentPath.pop_back();
		}
	}

	/**
	* Takes the specified bond data and returns the requested number of top and bottom results, along with the number of
	* maximal tenor choice paths (that is, paths which cannot be extended any further), using a DFS.
	*
	* Unlike the dynamic programming approach, here we cannot wait, all bond purchases must be contiguous.
	*/
	std::tuple<ExtremeList, ExtremeList, int>
	topBotCRFs(const BondReturnData& tenorData, const int numTopChoices, const int numBotChoices) {
		if (numTopChoices < 0 || numBotChoices < 0) {
			throw std::invalid_argument("topBotCRFs: The numbers of top/bottom results requested must be positive");
		}

		auto topChoices = ExtremeList(TopOrBottom::Top(), numTopChoices);
		auto botChoices = ExtremeList(TopOrBottom::Bottom(), numBotChoices);

		int numSolutions = 0;

		// We store the working path for the recursion here and work with it by reference to avoid unnecessary copying.
		std::vector<int> currentPath;
		currentPath.reserve(tenorData.numMonths() / tenorData.tenors().front());

		recurseCRFs(tenorData, 0, 1, currentPath, topChoices, botChoices, numSolutions);

		return { topChoices, botChoices, numSolutions };
	}
}