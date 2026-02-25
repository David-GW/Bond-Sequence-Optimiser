#ifndef BSO_APP_DOMAIN_BOND_RETURN_DATA_HPP
#define BSO_APP_DOMAIN_BOND_RETURN_DATA_HPP

#include <cstddef>
#include <filesystem>
#include <mdspan>
#include <vector>

namespace Domain
{
	/// Stores the bond return data loaded from the provided file, along with the path to that file,
	/// allowing the tenors, number of months, and bond return values to be accessed.
	class BondReturnData
	{
		public:
			// Constructor:
			BondReturnData(std::vector<int> t, int m, std::vector<double> g, std::filesystem::path s);

			// Defined explicit move/copy semantics to avoid dangling mdspans.
			BondReturnData(const BondReturnData& other);
			BondReturnData& operator=(const BondReturnData& other);
			BondReturnData(BondReturnData&& other) noexcept;
			BondReturnData& operator=(BondReturnData&& other) noexcept;

			/**
			* Getter for bond return values of a BondReturnData grid in the natural way,
			* taking the row (of the desired tenor in the sorted grid) and month as (row, month).
			* This is designed for speed, and so does no bounds-checking, for this use .at(row, month).
			*
			* While stored as a 1D-vector for speed, this allows access as if grid were a 2D-vector,
			* using an mdspan for convenience.
			*/
			[[nodiscard]] double operator()(const int row, const int month) const {
				return gridView_[row, month];
			}

			/// Bounds-checking version of () getter.
			[[nodiscard]] double at(int row, int month) const;

			// Other getters:
			[[nodiscard]] const std::vector<int>& tenors() const noexcept { return tenors_; }
			[[nodiscard]] int numMonths() const noexcept { return numMonths_; }
			[[nodiscard]] const std::filesystem::path& dataPath() const noexcept { return dataPath_; }

			[[nodiscard]] int numTenors() const noexcept {
				return static_cast<int>(tenors_.size());
			}

		private:
			std::vector<int> tenors_;
			int numMonths_;
			// A row-major vectorisation of the grid of bond return data, with rows sorted by increasing tenor.
			std::vector<double> grid_;
			// An mdspan view over "grid_" for convenient access.
			std::mdspan<double, std::dextents<std::size_t, 2>> gridView_;
			std::filesystem::path dataPath_;
	};
}

#endif // BSO_APP_DOMAIN_BOND_RETURN_DATA_HPP