#include "cli/OutputMessages.h"

#include "helpers/OutputHelpers.h"

#include "types/RankingTypes.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <print>

namespace OutputMessages
{
	/// Prints an ExtremeList of top/bottom results, accounting for the actual number of solutions found.
	void printExtremeResults(const ExtremeList& extremeList, const int numSolutions) {
		if (numSolutions == 0) {
			throw NoSolutionsError();
		}

		const int numToPrint = static_cast<int>(extremeList.listLength());
		const auto& elemList = extremeList.rankedList();
		const double defaultValue = extremeList.listDefault();

		if (numToPrint > 0) {
			if (numToPrint == 1) {
				std::println("{} cumulative return and tenor choices:", extremeList.rankingType());
				std::println("{}", elemList.front());
			}
			else {
				// It may be that the user asks for more results than there are solutions.
				const int actualPrintNum = std::min(numToPrint, numSolutions);

				// Begin printing results.
				std::println("{} {} cumulative returns and tenor choices:", extremeList.rankingType(), actualPrintNum);
				for (const auto& choice : elemList) {
					// If we reach the default value of this list, we know we need to stop, there are no more solutions.
					if (choice.CRF() == defaultValue) break;

					std::println("{}", choice);
				}

				// Notify the user if they requested too many results.
				if (actualPrintNum < numToPrint) {
					if (actualPrintNum == 1) {
						std::println("NOTE: {} {} results requested, but only 1 solution exists",
							extremeList.rankingType(), numToPrint);
					}
					else {
						std::println("NOTE: {} {} results requested, but only {} solutions exist",
							extremeList.rankingType(), numToPrint, actualPrintNum);
					}
				}
			}
		}
	}

	/// Prints an example of the format of the bond data file.
	void printExampleCSV() {
		std::println("Tenor, 0, 1, 2, ...");
		std::println("3, 0.03197, 0.03225, 0.03179, ...");
		std::println("6, 0.06517, 0.06606, 0.06554, ...");
		std::println("...");
	}

	/// Prints an example of how the  bond data file should look if opened in spreadsheet software.
	void printExampleTable() {
		std::println("  Tenor  |    0    |    1    |    2    |  ...   ");
		std::println("---------+---------+---------+---------+--------");
		std::println("    3    | 0.03197 | 0.03225 | 0.03179 |  ...   ");
		std::println("---------+---------+---------+---------+--------");
		std::println("    6    | 0.06517 | 0.06606 | 0.06554 |  ...   ");
		std::println("---------+---------+---------+---------+--------");
		std::println("   ...   |         |         |         |        ");
	}

	/// Print comprehensive instructions on the required format of the bond data file.
	void printFileHelp() {
		std::println();
		OutputHelpers::printRule();
		std::println("FILE HELP");
		OutputHelpers::printRule();
		std::println();
		OutputHelpers::wrappedPrint("Bond return data should be provided as a CSV file (a .csv or .txt extension is required).");
		std::println();
		OutputHelpers::wrappedPrint("The first row should have as its first value the word \"Tenor\", "
			"followed by a consecutive list of months starting at 0.");
		std::println();
		OutputHelpers::wrappedPrint("Subsequent rows should begin with the tenor, and then list the return "
			"should a bond of that tenor be purchased in the month of the corresponding column.");
		std::println();
		std::println("For example, the CSV file should resemble:");
		std::println();
		printExampleCSV();
		std::println();
		std::println("(spaces are optional, and blank rows will be ignored).");
		std::println();
		OutputHelpers::wrappedPrint("If opened in spreadsheet software such as Excel, the data should resemble:");
		std::println();
		printExampleTable();
		std::println();
		OutputHelpers::wrappedPrint("but, if editing in such software, ensure that the file remains saved as .csv or .txt.");
		std::println();
		OutputHelpers::printRule();
	}
}