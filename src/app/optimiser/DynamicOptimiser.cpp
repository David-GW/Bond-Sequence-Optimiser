#include "app/optimiser/DynamicOptimiser.hpp"

#include "app/domain/BondReturnData.hpp"
#include "app/domain/InvestmentAction.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <limits>
#include <mdspan>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace DynamicOptimiser
{
    namespace Detail
    {
        using DecisionsList = std::vector<std::vector<Domain::InvestmentAction>>;
        // Type for the decisions mdspan:
        using DecisionsSpan =
            std::mdspan<int, std::extents<std::size_t, std::dynamic_extent, std::dynamic_extent, 2>>;

        namespace Overflow
        {
            static void throwCRFOverflow(const double value, const int month) {
                if (!std::signbit(value)) {
                    throw std::overflow_error(
                        std::format(
                            "return exceeding finite limit ({:.3e}) possible by month {}",
                            std::numeric_limits<double>::max(),
                            month
                        )
                    );
                }
                throw std::overflow_error(
                    std::format(
                        "return below finite limit ({:.3e}) possible by month {}",
                        std::numeric_limits<double>::lowest(),
                        month
                    )
                );
            }
        }

        namespace PriorityQueue
        {
            /// Stores the necessary data for each priority queue element in the k-way merge.
            struct Candidate
            {
                double CRF{}; // candidate cumulative return factor at current month
                int tenor{}; // 0 = wait, > 0 = buy tenor
                int prevRank{}; // rank in predecessor row

                // These are not strictly necessary, but are present to avoid unnecessary recomputation and branching:
                int prevMonth{}; // month of predecessor
                double factor{}; // return factor of predecessor
            };

            constexpr auto compareCandidates = [](const Candidate& a, const Candidate& b) {
                return a.CRF < b.CRF;
            };

            using CandidateQueue =
                std::priority_queue<Candidate, std::vector<Candidate>, decltype(compareCandidates)>;
        }

        namespace PathReconstruction
        {
            /// Reconstructs the paths of optimal investment decisions made by getOptimalSequences.
            [[nodiscard]] static DecisionsList reconstructPaths(
                const DecisionsSpan& decisions,
                const int numMonths,
                const int numResultsFound
            ) {
                DecisionsList pathsList{};
                pathsList.reserve(numResultsFound);

                for (int currentRank = 0; currentRank < numResultsFound; ++currentRank) {
                    // If this rank wasn't produced, stop:
                    if (decisions[numMonths, currentRank, 0] == -1) {
                        break;
                    }

                    std::vector<Domain::InvestmentAction> currentPath{};
                    int currentMonth = numMonths;
                    int prevRank = currentRank;
                    // Rather than add multiple 1-month waits, we keep track of contiguous waits and add this period
                    // as a single InvestmentAction.
                    int waitStreak = 0;

                    while (currentMonth > 0) {
                        const int tenorToReachMonth = decisions[currentMonth, prevRank, 0];
                        prevRank = decisions[currentMonth, prevRank, 1];
                        // 0 is wait sentinel.
                        if (tenorToReachMonth == 0) {
                            // Wait 1 month:
                            ++waitStreak;
                            --currentMonth;
                        }
                        else {
                            // Period of waiting has ended, so must add this to decision list:
                            if (waitStreak > 0) {
                                currentPath.emplace_back(
                                    Domain::InvestmentAction::Action::Wait,
                                    currentMonth,
                                    waitStreak
                                );
                                waitStreak = 0;
                            }
                            // Buy tenor starting from current month:
                            currentPath.emplace_back(
                                Domain::InvestmentAction::Action::Buy,
                                currentMonth,
                                tenorToReachMonth
                            );
                            currentMonth -= tenorToReachMonth;
                        }
                    }
                    // If we the path finished with waiting, we need to add this to the decision list too:
                    if (waitStreak > 0) {
                        currentPath.emplace_back(
                            Domain::InvestmentAction::Action::Wait,
                            0,
                            waitStreak
                        );
                    }
                    // Path was constructed in reverse to avoid using .insert() and constantly shuffling memory.
                    std::ranges::reverse(currentPath);
                    pathsList.push_back(std::move(currentPath));
                }
                return pathsList;
            }
        }
    }

//----------------------------------------------------------------------------------------------------------------------

    OptimalResults getOptimalSequences(const Domain::BondReturnData& tenorData, const int numResultsRequested) {
        const int numTenors = tenorData.numTenors();
        const int numMonths = tenorData.numMonths();

        const auto& tenorList = tenorData.tenors();

        // Should never happen with current input validation.
        if (numResultsRequested < 0) {
            throw std::invalid_argument("Cannot request a negative number of results");
        }

        // numMonths and numTenors should always be > 0 with current input validation.
        if (numResultsRequested == 0 || numMonths == 0 || numTenors == 0) {
            return {{}, {}};
        }

        // Stores the results once computed:
        std::vector<double> finalCRFs{};
        Detail::DecisionsList finalPaths{};

        int numResultsFound{};

        // This works since tenors are sorted at construction.
        const int maxTenor = tenorList.back();
        // When calculating CRFs, we only need to look back as far as the length of the longest tenor
        // (+ 1 since we also need to store the current month), and so we can use a window to save memory.
        // We can't do this with decisionsBuffer, since we need to reconstruct the full path later.
        const std::size_t window = static_cast<std::size_t>(std::min(maxTenor, numMonths)) + 1;
        // Since we are using a rolling window, we need to know the index of the true last month,
        // we can't just use the last month in CRFsBuffer since it may have wrapped.
        std::size_t finalRowPos{};

        // Stores the requested number of maximal CRFs for each month, to be accessed as CRFs[month, rank].
        // We use an mdspan over a flat, contiguous vector for speed.
        std::vector CRFsBuffer(window * numResultsRequested, -std::numeric_limits<double>::infinity());
        const std::mdspan CRFs(CRFsBuffer.data(), window, numResultsRequested);

        // We scope decisionsBuffer to ensure is destroyed via RAII as soon as possible, since it can be large.
        {
            // Stores the tenor chosen and the previous rank in the path so we can reconstruct the chain of purchases,
            // the tenor is accessed as decisions[month, rank, 0], and the previous rank as decisions[month, rank, 1].
            // 0 is a sentinel representing wait, since we don't need to purchase at any given month.
            // We use an mdspan over a flat, contiguous vector for speed.
            std::vector decisionsBuffer(
                (static_cast<std::size_t>(numMonths) + 1) * static_cast<std::size_t>(numResultsRequested) * 2,
                -1
            );
            const Detail::DecisionsSpan decisions(
                decisionsBuffer.data(),
                static_cast<std::size_t>(numMonths) + 1,
                numResultsRequested,
                2
            );

            // Base case:
            CRFs[0, 0] = 1.0; // return at month 0 is 1
            decisions[0, 0, 0] = 0; // seeded that we "waited" to reach month 0

            // Stores the index in the windowed CRFs span which corresponds to a given month.
            // We do this rather than using % for speed.
            std::vector<std::size_t> rowIndex(static_cast<std::size_t>(numMonths) + 1);
            std::size_t windowCounter = 1;

            for (int currentMonth = 1; currentMonth <= numMonths; ++currentMonth) {
                rowIndex[currentMonth] = windowCounter;
                if (++windowCounter == window) {
                    windowCounter = 0;
                }

                // Reset the current months values, since they will be stale after the window first wraps:
                for (int i = 0; i < numResultsRequested; ++i) {
                    CRFs[rowIndex[currentMonth], i] = -std::numeric_limits<double>::infinity();
                }

                // Build a heap of list heads: waiting + each tenor that can end at the current month
                Detail::PriorityQueue::CandidateQueue candidatePQ(Detail::PriorityQueue::compareCandidates);

                // Add the waiting head:
                int prevMonth = currentMonth - 1;
                candidatePQ.emplace(CRFs[rowIndex[prevMonth], 0], 0, 0, prevMonth, 1.0);

                // Add the tenors heads:
                for (int i = 0; i < numTenors; ++i) {
                    const int currentTenor = tenorList[i];
                    if (currentMonth < currentTenor) {
                        continue;
                    }
                    prevMonth = currentMonth - currentTenor;
                    const double factor = 1.0 + tenorData(i, prevMonth);
                    // Note: prevCRF will never be -inf here, since we allow waiting there will always be
                    // at least one way to reach each month.
                    const double prevCRF = CRFs[rowIndex[prevMonth], 0];
                    const double nextCRF = prevCRF * factor;

                    if (std::isinf(nextCRF)) {
                        Detail::Overflow::throwCRFOverflow(nextCRF, currentMonth);
                    }
                    candidatePQ.emplace(nextCRF, currentTenor, 0, prevMonth, factor);
                }

                // Extract the number of maximal results requested for this month:
                int numResults = 0;
                while (numResults < numResultsRequested && !candidatePQ.empty()) {
                    Detail::PriorityQueue::Candidate topCandidate = candidatePQ.top();
                    candidatePQ.pop();

                    CRFs[rowIndex[currentMonth], numResults] = topCandidate.CRF;
                    decisions[currentMonth, numResults, 0] = topCandidate.tenor;
                    decisions[currentMonth, numResults, 1] = topCandidate.prevRank;
                    ++numResults;

                    // Advance the list the current maximal head came from:
                    if (const int nextRank = topCandidate.prevRank + 1; nextRank < numResultsRequested) {
                        const double prevCRF = CRFs[rowIndex[topCandidate.prevMonth], nextRank];
                        // Stop advancing if we reach the sentinel, no more results are available from that month.
                        if (prevCRF != -std::numeric_limits<double>::infinity()) {
                            const double nextCRF = prevCRF * topCandidate.factor;
                            if (std::isinf(nextCRF)) {
                                Detail::Overflow::throwCRFOverflow(nextCRF, currentMonth);
                            }
                            candidatePQ.emplace(
                                nextCRF, topCandidate.tenor, nextRank, topCandidate.prevMonth, topCandidate.factor
                            );
                        }
                    }
                }
                numResultsFound = numResults;

                // Any unfilled tail remains at -inf CRF and {-1, -1} decision.
            }
            finalPaths = Detail::PathReconstruction::reconstructPaths(decisions, numMonths, numResultsFound);
            finalRowPos = rowIndex[numMonths];
        }

        // Return last row of CRFs as a vector (note that numResultsFound will hold the value for the final month):
        finalCRFs.reserve(numResultsFound);
        for (int i = 0; i < numResultsFound; ++i) {
            finalCRFs.push_back(CRFs[finalRowPos, i]);
        }

        return OptimalResults{
            .CRFs = std::move(finalCRFs),
            .decisions = std::move(finalPaths)
        };
    }
}