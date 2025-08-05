#ifndef RANKING_TYPES_H
#define RANKING_TYPES_H

#include "CRFAndChoices.h"

#include <cstddef>
#include <format>
#include <limits>
#include <ostream>
#include <vector>

/**
* Used to store, semantically, whether we are dealing with the top or bottom results in an ExtremeList,
* to avoid using arbitrary flags such as true for top and false for bottom
*/
struct TopOrBottom
{
	enum class Kind { Top, Bottom };

	const Kind kind;

	[[nodiscard]] static TopOrBottom Top() { return {Kind::Top}; }
	[[nodiscard]] static TopOrBottom Bottom() { return {Kind::Bottom}; }

	// The behaviour of the << operator will be defined using the formatter defined later.
	friend std::ostream& operator<<(std::ostream&, const TopOrBottom&);
};

/// Tells std::format to output the Kind when passed a TopOrBottom.
template <>
struct std::formatter<TopOrBottom, char>
{
	static constexpr auto parse(const std::format_parse_context& ctx) {
		return ctx.begin(); // no custom formatting options
	}

	static auto format(const TopOrBottom& topOrBot, std::format_context& ctx) {
		if (topOrBot.kind == TopOrBottom::Kind::Top) {
			return std::format_to(ctx.out(), "Top");
		}
		return std::format_to(ctx.out(), "Bottom");
	}
};

/// Tells << to output the Kind when passed a TopOrBottom, by making use of the existing behaviour defined for std::format.
inline std::ostream& operator<<(std::ostream& os, const TopOrBottom& topOrBot) {
	return os << std::format("{}", topOrBot); // use existing formatter
}

/// Allows the specification of lists of top/bottom results of a given length, stored as initialised lists of CRFAndChoices.
class ExtremeList
{
	public:
		explicit ExtremeList(const TopOrBottom topOrBottom, const int numEntries):
			rankingKind_(topOrBottom)
		{
			if (numEntries < 0) {
				throw std::invalid_argument("ExtremeList: List length cannot be negative");
			}
			const double defaultValue = listDefault();
			rankedList_.assign(static_cast<std::size_t>(numEntries),CRFAndChoices(defaultValue, {}));
		}

		// Getters:
		[[nodiscard]] TopOrBottom rankingType() const noexcept { return rankingKind_; }
		[[nodiscard]] TopOrBottom& rankingKind() noexcept { return rankingKind_; }
		[[nodiscard]] const std::vector<CRFAndChoices>& rankedList() const noexcept { return rankedList_; }
		[[nodiscard]] std::vector<CRFAndChoices>& rankedList() noexcept { return rankedList_; }

		// To avoid needing to store numEntries after construction, we can deduce it from the length of the ranked list.
		[[nodiscard]] std::size_t listLength() const noexcept { return rankedList_.size(); }

		// Any CRF needs to beat the default in the list rankings, and so we must initialise the CRFs of the list values to ∓∞.
		[[nodiscard]] double listDefault() const {
			if (rankingKind_.kind == TopOrBottom::Kind::Top) {
				return -std::numeric_limits<double>::infinity();
			}
			return std::numeric_limits<double>::infinity();
		}

	private:
		TopOrBottom rankingKind_; // whether we are looking at the top or bottom results
		std::vector<CRFAndChoices> rankedList_; // the list of results
};

#endif // RANKING_TYPES_H