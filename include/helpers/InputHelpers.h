#ifndef INPUT_HELPERS_H
#define INPUT_HELPERS_H

#include "helpers/OutputHelpers.h"

#include <concepts>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <sstream>
#include <optional>
#include <print>
#include <locale>
#include <stdexcept>


/// Contains functions which mainly help to get input from the terminal.
namespace InputHelpers
{
	// Implemented in "src/helpers/InputHelpers.cpp".
	[[nodiscard]] std::string getExtension(const std::string& pathString);

	/// Ensures that T can read values from a stream.
	template<typename T>
	concept StreamReadable = requires(std::istream& is, T& value) {
		{ is >> value } -> std::same_as<std::istream&>;
	};

	/// Ensures that F is a function which takes the type of Arg as its argument and returns something evaluable as a bool.
	template<typename F, typename Arg>
	concept ValidatorFor = std::predicate<F, Arg>;

	/**
	* Prompts the user for data streamed from the terminal into a type T until it passes the specified validation,
	* throwing the specified error message if this validation fails. Once the validation passes then the T valued result is returned.
	*
	* However, if the user enters an empty string, then promptValidated returns without a value.
	*/
	template<
		StreamReadable T,
		ValidatorFor<std::string_view> Validator = decltype([](std::string_view) { return true; })
	>
	[[nodiscard]] std::optional<T> promptValidated(
		const std::string_view prompt,
		const Validator inputIsValid = [](std::string_view) { return true; },
		const std::string_view errorMessage = "Invalid entry"
	) {
		// To avoid user confusion, the prompt must describe something.
		if (prompt.empty()) {
			throw std::invalid_argument("promptValidated: Prompt must be non-empty");
		}

		// Loop until the user enters a valid string.
		while (true) {
			std::println("{}", prompt);
			std::string input;
			std::getline(std::cin, input);

			// In order to "escape", the user can enter an empty string (i.e. just press ENTER),
			// promptValidated then returns without a value, which can be handled by the caller.
			if (input.empty()) {
				return std::nullopt;
			}

			if (!inputIsValid(std::string_view(input))) {
				// The error message can be empty, in which case we avoid printing a new line.
				OutputHelpers::printIfNotEmpty(std::cerr, errorMessage);
				std::println();
				continue;
			}

			// Prepare the stream to be read into a value of type T.
			std::istringstream iss(input);
			// Setting the local ensures consistency of parsing regardless of the users system local,
			// e.g. by forcing "." to be used as the decimal seperator rather than "," as in some regions.
			iss.imbue(std::locale::classic());

			T value;
			// Try to read the stream into our value to return.
			if (!(iss >> value)) {
				// Again, only print if the error message is non-empty.
				OutputHelpers::printIfNotEmpty(std::cerr, errorMessage);
				continue;
			}
			return std::optional<T>{value};
		}
	}
}

#endif // INPUT_HELPERS_H