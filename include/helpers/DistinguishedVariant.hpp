#ifndef BSO_HELPERS_DISTINGUISHED_VARIANT_HPP
#define BSO_HELPERS_DISTINGUISHED_VARIANT_HPP

#include <concepts>
#include <stdexcept>
#include <utility>
#include <variant>

namespace Helpers::Variant
{
	/// An equivalent of std::bad_optional_access for the DistinguishedVariant class.
	struct BadDistinguishedAccess final : std::logic_error
	{
		using std::logic_error::logic_error;
	};

//----------------------------------------------------------------------------------------------------------------------

	/// A variant-like class with a distinguished "special" type with dedicated getters and probes,
	/// conceptually similar to std::expected, but with multiple unexpected types.
	template<typename T, typename... Others>
	class DistinguishedVariant
	{
		using variant_t = std::variant<T, Others...>;

		public:
			using value_t = T;

			template<typename U>
			requires std::constructible_from<variant_t, U>
			DistinguishedVariant(U&& v) :
				_v(std::forward<U>(v))
			{}

			// We use deducing this to avoid specifying separate getters for const, &, etc.

			template<typename Self>
			[[nodiscard]] decltype(auto) getValue(this Self&& self) {
				if (!self.template is<value_t>()) {
					throw BadDistinguishedAccess{"DistinguishedVariant does not hold distinguished type"};
				}
				return self.template get<value_t>();
			}

			template<typename U, typename Self>
			[[nodiscard]] decltype(auto) get(this Self&& self) {
				if (!self.template is<U>()) {
					throw BadDistinguishedAccess{"DistinguishedVariant does not hold type requested"};
				}
				return std::get<U>(std::forward_like<Self>(self._v));
			}

			[[nodiscard]] bool isValue() const noexcept {
				return this->is<value_t>();
			}

			template<typename U>
			[[nodiscard]] bool is() const noexcept {
				return std::holds_alternative<U>(_v);
			}

		protected:
			variant_t _v;
	};
}

#endif // BSO_HELPERS_DISTINGUISHED_VARIANT_HPP