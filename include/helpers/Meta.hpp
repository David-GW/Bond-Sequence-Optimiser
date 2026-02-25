#ifndef BSO_HELPERS_META_HPP
#define BSO_HELPERS_META_HPP

#include <concepts>
#include <type_traits>

namespace Helpers::Meta
{
	// Standard overload-set helper for std::visit: combines multiple lambdas/functors into one callable object
	// with all operator() overloads. See (for example):
	// https://www.modernescpp.com/index.php/visiting-a-std-variant-with-the-overload-pattern/.

	template<class... Ts>
	struct overloaded : Ts... { using Ts::operator()...; };

	template<class... Ts>
	overloaded(Ts...) -> overloaded<Ts...>;

//----------------------------------------------------------------------------------------------------------------------

	namespace Detail
	{
		template<class T, template<class...> class Template>
		struct is_specialisation_of : std::false_type {};

		template<template<class...> class Template, class... Args>
		struct is_specialisation_of<Template<Args...>, Template> : std::true_type {};
	}

	/// Checks if the first given type is a specialisation of the second,
	/// for example std::vector<int> is a specialisation of std::vector.
	template<class T, template<class...> class Template>
	inline constexpr bool isSpecialisationOf = Detail::is_specialisation_of<std::remove_cvref_t<T>, Template>::value;

	/// Requires that the first given type is a specialisation of the second.
	template<class T, template<class...> class Template>
	concept SpecialisationOf = isSpecialisationOf<T, Template>;
}

#endif // BSO_HELPERS_META_HPP