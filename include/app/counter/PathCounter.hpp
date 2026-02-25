#ifndef BSO_APP_COUNTER_PATH_COUNTER_HPP
#define BSO_APP_COUNTER_PATH_COUNTER_HPP

#include <vector>

namespace PathCounter
{
	/// Prints the total number of possible buying strategies from the provided bond return data,
	/// printing an approximation if the number of strategies exceeds exact representation as a long long.
	void printPathCount(const std::vector<int>& tenorList, int numMonths);
}

#endif // BSO_APP_COUNTER_PATH_COUNTER_HPP