#ifndef BSO_APP_IO_CSV_LOADER_HPP
#define BSO_APP_IO_CSV_LOADER_HPP

#include <stdexcept>
#include <string_view>

namespace Domain
{
	// Forward declaration, implemented in "include/app/domain/BondReturnData.hpp".
	class BondReturnData;
}

namespace IO::Input
{
	/// Thrown if an error occurs loading the provided file or parsing its data.
	struct CSVError final : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	/// Takes a provided file path string, checks the file contains bond return data in the required format,
	/// and returns the data as BondReturnData.
	[[nodiscard]] Domain::BondReturnData loadBondReturnCSV(std::string_view CSVPathSv);
}

#endif // BSO_APP_IO_CSV_LOADER_HPP