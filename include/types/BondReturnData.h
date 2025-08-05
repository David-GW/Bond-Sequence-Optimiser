#ifndef BOND_RETURN_DATA_H
#define BOND_RETURN_DATA_H

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

/**
* Stores the bond return data loaded from the provided file, allowing the tenors, number of months, and
* bond return values to be accessed.
*/
class BondReturnData
{
	public:
		// Implemented in "src/types/BondReturnData.cpp".
		BondReturnData(std::vector<int> t, std::size_t m, std::vector<double> g);

		// Implemented in "src/types/BondReturnData.cpp".
		[[nodiscard]] double operator()(int row, int month) const;

		// Getters:
		[[nodiscard]] const std::vector<int>& tenors() const noexcept { return tenors_; }
		[[nodiscard]] std::size_t numMonths() const noexcept { return numMonths_; }

	private:
		// A list of tenors the tenors loaded from the provided file.
		std::vector<int> tenors_;
		// The total number of months of data found in the provided file.
		std::size_t numMonths_;
		// A row-major vectorisation of the grid of bond return data, with rows sorted by increasing tenor.
		std::vector<double> grid_;
		// Pointers to the start of each row of data in the grid.
		std::vector<const double*> rowPointers_;
};

/// Contains the machinery necessary to safely load the bond return data from the provided file.
namespace CSVLoader
{
	/// Thrown by loadBondReturnCSV if an error occurs loading the provided file or parsing its data.
	struct FileError final : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	// Implemented in "src/types/BondReturnData.cpp".
	[[nodiscard]] BondReturnData loadBondReturnCSV(const std::string& filePath);
}

#endif // BOND_RETURN_DATA_H