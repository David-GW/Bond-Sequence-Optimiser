#include "app/cli/OutputMessages.hpp"

#include "helpers/printing/Print.hpp"

#include <print>

namespace OutputMessages
{
	namespace Detail
	{
		/// Prints an example of the format of the bond data file.
		static void printExampleCSV() {
			std::println("Tenor, 0, 1, 2, ...");
			std::println("3, 0.03197, 0.03225, 0.03179, ...");
			std::println("6, 0.06517, 0.06606, 0.06554, ...");
			std::println("...");
		}

		/// Prints an example of how the  bond data file should look if opened in spreadsheet software.
		static void printExampleTable() {
			std::println("  Tenor  |    0    |    1    |    2    |  ...   ");
			std::println("---------+---------+---------+---------+--------");
			std::println("    3    | 0.03197 | 0.03225 | 0.03179 |  ...   ");
			std::println("---------+---------+---------+---------+--------");
			std::println("    6    | 0.06517 | 0.06606 | 0.06554 |  ...   ");
			std::println("---------+---------+---------+---------+--------");
			std::println("   ...   |   ...   |   ...   |   ...   |  ...   ");
		}
	}

	void printFileHelp() {
		std::println();
		Helpers::Printing::printRule();
		std::println("FILE HELP");
		Helpers::Printing::printRule();
		std::println();
		Helpers::Printing::wrappedPrintln(
			"Bond return data should be provided as a CSV file (a .csv or .txt extension is required)."
		);
		std::println();
		Helpers::Printing::wrappedPrintln("The first row should have as its first value the word \"Tenor\", "
			"followed by a consecutive list of months starting at 0.");
		std::println();
		Helpers::Printing::wrappedPrintln("Subsequent rows should begin with the tenor, and then list the "
			"return yielded by a bond of that tenor purchased in the month of the corresponding column.");
		std::println();
		std::println("For example, the CSV file should resemble:");
		std::println();
		Detail::printExampleCSV();
		std::println();
		std::println("(spaces are optional, and blank rows will be ignored).");
		std::println();
		Helpers::Printing::wrappedPrintln("If opened in spreadsheet software such as Excel, "
			"the data should resemble:");
		std::println();
		Detail::printExampleTable();
		std::println();
		Helpers::Printing::wrappedPrintln("but, if editing in such software, "
			"ensure that the file remains saved as .csv or .txt.");
		std::println();
		Helpers::Printing::printRule();
	}
}