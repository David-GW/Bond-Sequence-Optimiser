#include "helpers/Filesystem.hpp"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#if defined(_WIN32) || defined(_WIN64)
	#define IS_WINDOWS 1
#else
	#define IS_WINDOWS 0
#endif

namespace Helpers::Filesystem
{
	std::string getExtension(const std::filesystem::path& filePath) {
		auto ext = filePath.extension().string();
		if (ext.starts_with('.')) {
			ext.erase(0, 1);
		}
		// If no extension found, "" will be returned.
		return ext;
	}

	std::filesystem::path getDirectory(const std::filesystem::path& filePath) {
		auto dir = filePath.parent_path();
		if (dir.empty()) {
			dir = std::filesystem::current_path();
		}
		return dir.lexically_normal();
	}

	void assertDirectoryValid(const std::filesystem::path& dirPath) {
		std::error_code ec;
		if (!std::filesystem::exists(dirPath, ec)) {
			throw DirectoryError(std::format("\n{}\ndoes not exist", dirPath.string()));
		}
		if (ec) {
			throw DirectoryError(std::format("cannot access \n{}\n{}", dirPath.string(), ec.message()));
		}
		if (!std::filesystem::is_directory(dirPath, ec)) {
			if (ec) {
				throw DirectoryError(std::format("problem checking \n{}\n{}", dirPath.string(), ec.message()));
			}
			throw DirectoryError(std::format("\n{}\nis not a directory", dirPath.string()));
		}
	}

	void assertFileValid(const std::filesystem::path& filePath) {
		if (std::error_code ec; !std::filesystem::is_regular_file(filePath, ec)) {
			if (ec == std::errc::no_such_file_or_directory) {
				throw FileError(std::format("\n{}\ndoes not exist", filePath.string()));
			}
			if (ec) {
				throw FileError(std::format("\n{}\nis unreadable:\n{}", filePath.string(), ec.message()));
			}
			if (std::error_code ecDir; std::filesystem::is_directory(filePath, ecDir)) {
				throw FileError(std::format("\n{}\nis a directory, not a file", filePath.string()));
			}
			throw FileError(std::format("\n{}\nis not a regular file:\n{}", filePath.string(), ec.message()));
		}
	}

	std::filesystem::path expandUserPath(const std::string_view pathSv) {
		if (pathSv.empty() || pathSv.front() != '~') {
			return std::filesystem::path{pathSv}.lexically_normal();
		}

		#if IS_WINDOWS
		const char* home = std::getenv("USERPROFILE");
		#else
		const char* home = std::getenv("HOME");
		#endif
		if (!home) {
			throw DirectoryError("cannot expand '~': HOME environment variable not set");
		}

		std::filesystem::path expanded{home};
		if (pathSv.size() > 1) {
			if (pathSv[1] == '/' || pathSv[1] == '\\') {
				expanded /= std::filesystem::path{pathSv.substr(2)};
			}
			else {
				const std::size_t nextSlash = pathSv.find_first_of("/\\", 1);
				const std::string_view userPart = pathSv.substr(0, nextSlash);
				throw DirectoryError(
					std::format(
						"cannot expand \"{}\" (note that expansion of '~username' is not supported)",
						userPart
					)
				);
			}
		}
		return expanded.lexically_normal();
	}
}