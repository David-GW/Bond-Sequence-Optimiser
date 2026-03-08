#ifndef BSO_TRANSFORMERS_GENERIC_HPP
#define BSO_TRANSFORMERS_GENERIC_HPP

#include "helpers/DistinguishedVariant.hpp"
#include "helpers/Meta.hpp"
#include "helpers/Strings.hpp"
#include "helpers/printing/StyledPrint.hpp"

#include <concepts>
#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace Transformers
{
	/// Stores the message and its style when prompting the user to retry entering input to the transformer.
	struct Retry
	{
		std::string message{};
		Helpers::Printing::Style style;

		template <Helpers::Strings::StringLike S>
		explicit Retry(S&& msg, const Helpers::Printing::Style s = Helpers::Printing::Styles::error) :
			message(std::string_view(msg)),
			style(s)
		{}

		Retry() = default;
	};

	/// Indicates that the prompt has been requested to terminate without returning a result.
	struct Escape{};

	template<typename T>
	using TransformerBase = Helpers::Variant::DistinguishedVariant<T, Retry, Escape>;

	/// Stores the output from a transformer, extending the DistinguishedVariant class with specific probes
	/// for checking if the prompt has been requested to terminate or if the user must retry entering input,
	/// as well as getters for the message and style in case the user must retry.
	template<typename T>
	class TransformerResult : public TransformerBase<T>
	{
		using Base = TransformerBase<T>;

		public:
			using Base::Base;

			[[nodiscard]] bool isRetry()  const noexcept {
				return this->template is<Retry>();
			}
			[[nodiscard]] bool isEscape() const noexcept {
				return this->template is<Escape>();
			}

			[[nodiscard]] const std::string& retryMessage() const {
				return std::get<Retry>(this->_v).message;
			}
			[[nodiscard]] const Helpers::Printing::Style& retryStyle() const {
				return std::get<Retry>(this->_v).style;
			}
	};

//----------------------------------------------------------------------------------------------------------------------

	namespace Detail
	{
		struct PromptEscape {};
	}

	template<typename T>
	using PromptBase = Helpers::Variant::DistinguishedVariant<T, Detail::PromptEscape>;

	/// Stores the final output of a prompt: either a value was returned or it was aborted,
	/// extending DistinguishedVariant with a specific probe to see if this is the case.
	template<typename T>
	class PromptResult : public PromptBase<T>
	{
		using Base = PromptBase<T>;

		public:
			using Base::Base;

			[[nodiscard]] bool isEscape() const noexcept {
				return this->template is<Detail::PromptEscape>();
			}
	};

//----------------------------------------------------------------------------------------------------------------------

	/// Ensures that the function being passed with the prompt returns some type of TransformerResult.
	template <typename F>
	concept TransformerCallable = Helpers::Meta::SpecialisationOf<
		std::invoke_result_t<F, std::string_view>,
		TransformerResult
	>;

	/// Prompts the user with the specified message until the transformer returns a result or is asked
	/// to escape, printing the retry message styled as specified before each reattempt.
	/// Here templating allows the return type to be deduced from that of the given transformer.
	template <TransformerCallable F>
	[[nodiscard]] PromptResult<typename std::invoke_result_t<F, std::string_view>::value_t>
	promptTransformer(std::string_view prompt, F&& transformer) {
		std::string input{};

		while (true) {
			std::println("{}", prompt);

			if (!std::getline(std::cin, input)) {
				// EOF behaves like escape.
				return Detail::PromptEscape{};
			}

			auto result = transformer(input);

			if (result.isValue()) {
				return std::move(result).getValue();
			}
			if (result.isRetry()) {
				if (!result.retryMessage().empty()) {
					Helpers::Printing::styledPrintln(result.retryStyle(),"{}", result.retryMessage());
				}
				std::println();
				continue;
			}
			if (result.isEscape()) {
				return Detail::PromptEscape{};
			}
			std::unreachable(); // TransformerResult holds exactly T, Retry, or Escape
		}
	}
}

#endif // BSO_TRANSFORMERS_GENERIC_HPP