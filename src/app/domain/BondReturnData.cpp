#include "app/domain/BondReturnData.hpp"

#include <cstddef>
#include <filesystem>
#include <limits>
#include <mdspan>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Domain
{
	BondReturnData::BondReturnData(std::vector<int> t, const int m, std::vector<double> g, std::filesystem::path s) :
		tenors_(std::move(t)),
		numMonths_(m),
		grid_(std::move(g)),
		gridView_{grid_.data(), tenors_.size(), numMonths_},
		dataPath_(std::move(s))
	{
		if (tenors_.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
			throw std::invalid_argument("BondReturnData: too many tenors provided");
		}

		if (numMonths_ <= 0) {
			throw std::invalid_argument("BondReturnData: must have at least 1 month");
		}

		if (grid_.size() != tenors_.size() * static_cast<std::size_t>(numMonths_)) {
			throw std::invalid_argument("BondReturnData: size mismatch");
		}
	}

//----------------------------------------------------------------------------------------------------------------------

	BondReturnData::BondReturnData(const BondReturnData& other):
		tenors_(other.tenors_),
		numMonths_(other.numMonths_),
		grid_(other.grid_),
		gridView_(grid_.data(), other.tenors_.size(), other.numMonths_),
		dataPath_(other.dataPath_)
	{}

	BondReturnData& BondReturnData::operator=(const BondReturnData& other)
	{
		if (this != &other) {
			tenors_ = other.tenors_;
			numMonths_ = other.numMonths_;
			grid_ = other.grid_;
			dataPath_ = other.dataPath_;
			gridView_ = decltype(gridView_)(grid_.data(), tenors_.size(), numMonths_);
		}
		return *this;
	}

	BondReturnData::BondReturnData(BondReturnData&& other) noexcept:
		tenors_(std::move(other.tenors_)),
		numMonths_(other.numMonths_),
		grid_(std::move(other.grid_)),
		gridView_(grid_.data(), tenors_.size(), numMonths_),
		dataPath_(std::move(other.dataPath_))
	{}

	BondReturnData& BondReturnData::operator=(BondReturnData&& other) noexcept
	{
		if (this != &other) {
			tenors_ = std::move(other.tenors_);
			numMonths_ = other.numMonths_;
			grid_ = std::move(other.grid_);
			dataPath_ = std::move(other.dataPath_);
			gridView_ = decltype(gridView_)(grid_.data(), tenors_.size(), numMonths_);
		}
		return *this;
	}

//----------------------------------------------------------------------------------------------------------------------

	double BondReturnData::at(const int row, const int month) const {
		if (row < 0 || row >= numTenors()) {
			throw std::out_of_range("BondReturnData: row out of range");
		}
		if (month < 0 || month >= numMonths_) {
			throw std::out_of_range("BondReturnData: month out of range");
		}
		return gridView_[row, month];
	}
}