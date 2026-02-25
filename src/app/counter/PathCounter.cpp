#include "app/counter/PathCounter.hpp"

#include "helpers/Meta.hpp"
#include "helpers/Strings.hpp"

#include <cmath>
#include <limits>
#include <print>
#include <variant>
#include <vector>

namespace PathCounter
{
	namespace Detail
	{
		struct ExactCount {
			long long value{};
		};
		struct ApproxCount {
			double value{};
		};
		/// The number of buying strategies may exceed exact representation as a long long,
		/// if so we store an approximate double instead.
		using PathCount = std::variant<ExactCount, ApproxCount>;

		/// Uses dynamic programming to calculate the total number of possible buying strategies
		/// from the provided bond return data, returning printing an approximation if the number of strategies
		/// exceeds exact representation as a long long.
		PathCount countPaths(std::vector<int> tenorList, const int numMonths) {
			tenorList.insert(tenorList.begin(), 1);

			std::vector<long long> numPaths( numMonths + 1, 0);
			numPaths[0] = 1;

			std::vector<double> numPathsApprox;

			bool overflowed = false;

			for (int i = 1; i < numMonths + 1; ++i) {
				for (const auto t : tenorList) {
					if (t > i) {
						break;
					}
					if (!overflowed) {
						// Check for overflow before adding
						if (std::numeric_limits<long long>::max() - numPaths[i] < numPaths[i - t]) {
							// Switch to approximate mode
							overflowed = true;

							// Initialise approximate vector from exact values
							numPathsApprox.resize(numMonths + 1);
							for (int j = 0; j < i; ++j) {
								numPathsApprox[j] = static_cast<double>(numPaths[j]);
							}
							numPathsApprox[i] = static_cast<double>(numPaths[i]) + static_cast<double>(numPaths[i - t]);
						}
						else {
							numPaths[i] += numPaths[i - t];
						}
					}
					else {
						numPathsApprox[i] += numPathsApprox[i - t];
					}
				}
			}
			if (overflowed) {
				return ApproxCount{numPathsApprox.back()};
			}
			return ExactCount{numPaths.back()};
		}
	}

	void printPathCount(const std::vector<int>& tenorList, const int numMonths) {
		const Detail::PathCount count = Detail::countPaths(tenorList, numMonths);
		std::visit(
			Helpers::Meta::overloaded{
				[](const Detail::ApproxCount& approx) {
					constexpr auto maxExactDoubleInt = static_cast<double>(1ULL << std::numeric_limits<double>::digits);
					if (const double dCount = approx.value; !std::isfinite(dCount)) {
						std::println("Over {:.3e}", std::numeric_limits<double>::max());
					}
					else if (dCount > maxExactDoubleInt) {
						std::println("{:.3e}", dCount);
					}
					else {
						std::println("{}", Helpers::Strings::formatIntWithSeparator(static_cast<long long>(dCount)));
					}
				},
				[](const Detail::ExactCount& exact) {
					std::println("{}", Helpers::Strings::formatIntWithSeparator(exact.value));
				}
			},
			count
		);
	}
}