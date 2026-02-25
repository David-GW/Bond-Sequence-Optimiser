#ifndef BSO_HELPERS_FILESYSTEM_HPP
#define BSO_HELPERS_FILESYSTEM_HPP

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Helpers::Filesystem
{
	// Errors thrown if any of the Filesystem helpers encounter problems with files or directories.

	struct FilesystemError : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	struct FileError final : FilesystemError
	{
		using FilesystemError::FilesystemError;
	};

	struct DirectoryError final : FilesystemError
	{
		using FilesystemError::FilesystemError;
	};

//----------------------------------------------------------------------------------------------------------------------

	[[nodiscard]] std::string getExtension(const std::filesystem::path& filePath);

	[[nodiscard]] std::filesystem::path getDirectory(const std::filesystem::path& filePath);

	/// Checks that the path is a directory which exists and is accessible.
	void assertDirectoryValid(const std::filesystem::path& dirPath);

	/// Checks that the path is a regular file which exists and is readable.
	void assertFileValid(const std::filesystem::path& filePath);

	[[nodiscard]] std::filesystem::path expandUserPath(std::string_view pathSv);
}

#endif // BSO_HELPERS_FILESYSTEM_HPP