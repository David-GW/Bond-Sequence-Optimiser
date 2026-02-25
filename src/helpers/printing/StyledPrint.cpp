#include "helpers/printing/StyledPrint.hpp"

#include "rang/include/rang.hpp"

#include <mutex>

namespace Helpers::Printing::Detail
{
	void rangConfigureOnce() {
		static std::once_flag once;
		std::call_once(once, [] {
			rang::setControlMode(rang::control::Auto);
			rang::setWinTermMode(rang::winTerm::Auto);
		});
	}
}