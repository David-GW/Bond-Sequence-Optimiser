#include "helpers/InputHelpers.h"

#include "helpers/GeneralHelpers.h"

#include <filesystem>
#include <string>

/// Contains functions which mainly help to get input from the terminal.
namespace InputHelpers
{
	/// Returns the file extension, without '.', and in lower case, of a given path/file name.
	std::string getExtension(const std::string& pathString) {
		const std::filesystem::path path(pathString);
		auto ext = path.extension().string();
		// Remove leading dot if necessary.
		if (ext.starts_with('.')) {
			return GeneralHelpers::svToLowercase(ext.substr(1));
		}
		// If no extension found, "" will be returned.
		return ext;
	}
}