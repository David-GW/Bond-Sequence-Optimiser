#ifndef CRF_AND_CHOICES_H
#define CRF_AND_CHOICES_H

#include <format>
#include <ostream>
#include <vector>

#include "helpers/GeneralHelpers.h"

/**
* Stores a CRF and an integer list of the tenors representing the buying strategy to achieve this.
*
* Used by the recursive algorithm topBotCRFs and stored in ExtremeLists.
*/
class CRFAndChoices
{
	public:
		CRFAndChoices(const double crf, std::vector<int> choices):
			CRF_(crf),
			tenorChoices_(std::move(choices))
		{}

		// Getters:
		[[nodiscard]] double CRF() const noexcept { return CRF_; }
		[[nodiscard]] const std::vector<int>& tenorChoices() const noexcept { return tenorChoices_; }

		// The behaviour of the << operator will be defined using the formatter defined later.
		friend std::ostream& operator<<(std::ostream&, const CRFAndChoices&);

	private:
		double CRF_;
		std::vector<int> tenorChoices_;
};

/**
* Tells std::format that, given a CRFAndChoices variable, it should derive the cumulative return to 2 decimal places from the CRF,
* and output this with the tenor choices as in the following example:
*
* "123.45%: { 3, 6, 3, 12, ..., 9 }"
*/
template <>
struct std::formatter<CRFAndChoices, char> {
	static constexpr auto parse(const std::format_parse_context& ctx) {
		return ctx.begin(); // no custom formatting options
	}

	static auto format(const CRFAndChoices& crfc, std::format_context& ctx) {
		return std::format_to(
			ctx.out(),
			"{:.2f}%: {{ {} }}",
			100 * crfc.CRF() - 100,
			GeneralHelpers::joinFormatted(crfc.tenorChoices())
		);
	}
};

/**
* Tells << that, given a CRFAndChoices variable, it should derive the cumulative return to 2 decimal places from the CRF,
* and output this with the tenor choices as in the following example:
*
* 123.45%: { 3, 6, 3, 12, ..., 9 }
*
* by making use of the existing behaviour defined for std::format.
*/
inline std::ostream& operator<<(std::ostream& os, const CRFAndChoices& crfc) {
	return os << std::format("{}", crfc); // use existing formatter
}

#endif // CRF_AND_CHOICES_H