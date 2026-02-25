#ifndef BSO_APP_DOMAIN_INVESTMENT_ACTION_HPP
#define BSO_APP_DOMAIN_INVESTMENT_ACTION_HPP

#include <format>
#include <iterator>
#include <ostream>
#include <stdexcept>

namespace Domain
{
	/// Stores an investment action: the starting month, whether to buy our wait, and the tenor of bond to buy
	/// or the length of time to wait.
	class InvestmentAction
	{
		public:
			enum class Action { Buy, Wait };

			// Constructor:
			explicit InvestmentAction(const Action actionInvestOrWait, const int choiceMonth, const int periodLength) :
				action_(actionInvestOrWait),
				startMonth_(choiceMonth),
				length_(periodLength)
			{
				if (choiceMonth < 0) {
					throw std::invalid_argument("InvestmentAction: Month cannot be negative");
				}
				if (periodLength <= 0) {
					throw std::invalid_argument("InvestmentAction: Tenor / wait length must be positive");
				}
			}

			// Getters:
			[[nodiscard]] Action action() const noexcept { return action_; }
			[[nodiscard]] int startMonth() const noexcept { return startMonth_; }
			[[nodiscard]] int length() const noexcept { return length_; }

			// Stream output according to the formatter defined later.
			friend std::ostream& operator<<(std::ostream&, const InvestmentAction&);

		private:
			Action action_;
			int startMonth_;
			int length_;
	};
}

/**
* Tells std::format to output an investment action as bn or wn for buying an n-month bond or waiting n months
* respectively.
*
* Also provides a verbose "v" option to output as in the following example:
*
* "Month x: buy y-month bond", or,  "Month x: wait for y months"
*/
template <>
struct std::formatter<Domain::InvestmentAction, char>
{
	bool verbose = false;

	constexpr auto parse(const std::format_parse_context& ctx) {
		auto it = ctx.begin();
		if (it != ctx.end() && *it == 'v') {
			verbose = true;
			++it;
		}
		return it;
	}

	template <class FormatContext>
	auto format(const Domain::InvestmentAction& choice, FormatContext& ctx) const {
		if (verbose) {
			if (choice.action() == Domain::InvestmentAction::Action::Buy) {
				return std::format_to(ctx.out(), "Month {}: buy {}-month bond", choice.startMonth(), choice.length());
			}
			if (choice.length() == 1) {
				return std::format_to(ctx.out(), "Month {}: wait for 1 month", choice.startMonth());
			}
			return std::format_to(ctx.out(), "Month {}: wait for {} months", choice.startMonth(), choice.length());
		}
		if (choice.action() == Domain::InvestmentAction::Action::Buy) {
			return std::format_to(ctx.out(), "b{}", choice.length());
		}
		return std::format_to(ctx.out(), "w{}", choice.length());
	}
};

namespace Domain
{
	inline std::ostream& operator<<(std::ostream& os, const InvestmentAction& choice) {
		std::format_to(std::ostream_iterator<char>(os), "{}", choice);
		return os;
	}
}

#endif // BSO_APP_DOMAIN_INVESTMENT_ACTION_HPP