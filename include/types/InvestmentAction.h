#ifndef INVESTMENT_ACTION_H
#define INVESTMENT_ACTION_H

#include <format>
#include <ostream>
#include <stdexcept>

/**
* Stores an investment action: the starting month, whether to buy our wait, and the tenor of bond to buy
* or the length of time to wait.
*
* Used by the dynamic programming algorithm optimiseCRF.
*/
class InvestmentAction
{
	public:
		enum class Action { Buy, Wait };

		InvestmentAction(const Action actionInvestOrWait, const int choiceMonth, const int periodLength):
			action_(actionInvestOrWait),
			startMonth_(choiceMonth),
			length_(periodLength)
		{
			if (choiceMonth < 0) {
				throw std::invalid_argument("InvestmentAction: Month cannot be negative");
			}
			if (periodLength <= 0) {
				throw std::invalid_argument("InvestmentAction: Tenor/wait length must be positive");
			}
		}

		// Getters:
		[[nodiscard]] Action action() const noexcept { return action_; }
		[[nodiscard]] int startMonth() const noexcept { return startMonth_; }
		[[nodiscard]] int length() const noexcept { return length_; }

		// The behaviour of the << operator will be defined using the formatter defined later.
		friend std::ostream& operator<<(std::ostream&, const InvestmentAction&);

	private:
		Action action_; // whether to buy or wait
		int startMonth_; // the month the action begins
		int length_; // the tenor of bond to buy, or length of time to wait
};

/**
* Tells std::format to output an investment action as in the following example:
*
* "Month x: buy y-month bond", or,  "Month x: wait for y months"
*/
template <>
struct std::formatter<InvestmentAction, char>
{
	static constexpr auto parse(const std::format_parse_context& ctx) {
		return ctx.begin(); // no custom format options
	}

	static auto format(const InvestmentAction& choice, std::format_context& ctx) {
		if (choice.action() == InvestmentAction::Action::Buy) {
			return std::format_to(ctx.out(), "Month {}: buy {}-month bond", choice.startMonth(), choice.length());
		}
		if (choice.length() == 1) {
			return std::format_to(ctx.out(), "Month {}: wait for 1 month", choice.startMonth());
		}
		return std::format_to(ctx.out(), "Month {}: wait for {} months", choice.startMonth(), choice.length());
	}
};

/**
* Tells << to output an investment action as in the following example:
*
* "Month x: buy y-month bond", or,  "Month x: wait for y months"
*
* by making use of the existing behaviour defined for std::format.
*/
inline std::ostream& operator<<(std::ostream& os, const InvestmentAction& choice) {
	return os << std::format("{}", choice); // use existing formatter
}

#endif // INVESTMENT_ACTION_H