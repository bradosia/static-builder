#define SETTINGS_FILE "static-builder-settings.json"
//#define DEBUG_FIND_FILE

// c++
#include <fstream>
#include <iostream>
#include <filesystem>
#include <memory>
#include <vector>
#include <string>

/* rapidjson v1.1 (2016-8-25)
 * Developed by Tencent
 */
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/reader.h" // rapidjson::ParseResult

/*
 * static-builder 1.0
 */
#include "static-builder/amalgamated.h"

namespace fs = std::filesystem;

int main(int argc, char **argv) {
	std::string libraryName = "libstatic.a";
	std::string CC = "gcc";
	std::string CFLAGS = "";
	rapidjson::Document d;
	std::vector<fs::path> directories, headerFiles, cppFiles, includePaths,
			libraryFiles;
	std::vector<std::vector<fs::path>> objectFilesBatch;
	unsigned int objectBatchNum = 14; // how many object files to "ar" at a time
	// Open up settings
	// 1. Parse a JSON string into DOM.
	std::fstream settingsFile;
	settingsFile.open(SETTINGS_FILE, std::fstream::in);
	if (settingsFile) {
		char *settingsFileChar;
		unsigned int fileSize;
		settingsFile.seekg(0, std::ios::end); // set the pointer to the end
		fileSize = settingsFile.tellg(); // get the length of the file
		settingsFile.seekg(0, std::ios::beg);
		settingsFileChar = new char[fileSize + 1];
		memset(settingsFileChar, 0, sizeof(settingsFileChar[0]) * fileSize + 1);
		settingsFile.read(settingsFileChar, fileSize);
		d.Parse(settingsFileChar);
		if (d.IsObject()) {
			if (d.HasMember("libraryName") && d["libraryName"].IsString()) {
				std::string temp = d["libraryName"].GetString();
				if (temp.length() > 0) {
					libraryName = temp;
				}
			}
			if (d.HasMember("includePaths") && d["includePaths"].IsArray()) {
				const rapidjson::Value &a = d["includePaths"];
				rapidjson::SizeType n = a.Size(); // rapidjson uses SizeType instead of size_t.
				for (rapidjson::SizeType i = 0; i < n; i++) {
					includePaths.emplace_back(a[i].GetString());
				}
			}
			if (d.HasMember("CC") && d["CC"].IsString()) {
				std::string temp = d["CC"].GetString();
				if (temp.length() > 0) {
					CC = temp;
				}
			}
			if (d.HasMember("CFLAGS") && d["CFLAGS"].IsString()) {
				std::string temp = d["CFLAGS"].GetString();
				if (temp.length() > 0) {
					CFLAGS = temp;
				}
			}
		}
		//delete settingsFileChar;
	}

	/*
	 * find files
	 */
	for (auto &p : fs::recursive_directory_iterator(".")) {
		std::string ext = p.path().extension().string();
		if (ext == ".cpp" || ext == ".cc" || ext == ".c") {
#ifdef DEBUG_FIND_FILE
			std::cout << "cpp\n";
#endif
			cppFiles.push_back(p.path());
		} else if (ext == ".h") {
#ifdef DEBUG_FIND_FILE
			std::cout << "header file\n";
#endif
			headerFiles.push_back(p.path());
		} else if (fs::is_directory(p)) {
#ifdef DEBUG_FIND_FILE
			std::cout << "directory\n";
#endif
			directories.push_back(p.path());
		}
#ifdef DEBUG_FIND_FILE
		 std::cout << p.path() << '\n';
#endif
	}

	/*
	 * build
	 */
	fs::create_directory("objects");
	// include string
	std::string includeString = "";
	for (fs::path &p : includePaths) {
		includeString.append(" -I\"").append(p.string()).append("\"");
	}
	for (fs::path &p : directories) {
		//includeString.append(" -I").append(p.string());
	}
	unsigned int batchNum = 0;
	objectFilesBatch.emplace_back();
	for (fs::path &p : cppFiles) {
		fs::path objectParentPath, objectFile;
		std::string cmd = "";
		std::string exec_output = "error";
		// object parent path
		objectParentPath = "objects" / removeParentDir(p.parent_path());
		fs::create_directories(objectParentPath);
		objectFile = objectParentPath
				/ std::string(p.stem().string()).append(".o");
		// 2>&1 at end will redirect stderr to stdout
		cmd.append(CC).append(" -c -o ").append(objectFile.string()).append(" ").append(
				p.string()).append(" ").append(CFLAGS).append(includeString).append(
				" 2>&1");
		std::cout << cmd << "\n";
		try {
			exec_output = exec(cmd);
			std::cout << "OUTPUT: " << exec_output << std::endl;
		} catch (const std::exception &e) {
			//std::cout << e.what() << std::endl << cmd << std::endl;

		}
		if (exec_output.length() == 0) {
			objectFilesBatch[batchNum].push_back(objectFile);
		}
		if (objectFilesBatch[batchNum].size() > objectBatchNum) {
			/*
			 * output libraries as they are batched so user can see progress
			 */
			std::vector<fs::path> &objectFiles = objectFilesBatch[batchNum];
			std::string cmd = "ar rvs ";
			std::string libraryFileName;
			libraryFileName.append("objects/lib").append(libraryName).append(
					std::to_string(batchNum)).append(".a");
			libraryFiles.emplace_back(libraryFileName);
			cmd.append(libraryFileName);
			for (fs::path &p : objectFiles) {
				cmd.append(" ").append(p.string());
			}
			std::cout << cmd << std::endl;
			std::cout << exec(cmd);

			batchNum++;
			objectFilesBatch.emplace_back();
		}
	}

	/*
	 * last one
	 */
	std::vector<fs::path> &objectFiles = objectFilesBatch[batchNum];
	std::string cmd = "ar rvs ";
	std::string libraryFileName;
	libraryFileName.append("objects/lib").append(libraryName).append(
			std::to_string(batchNum)).append(".a");
	libraryFiles.emplace_back(libraryFileName);
	cmd.append(libraryFileName);
	for (fs::path &p : objectFiles) {
		cmd.append(" ").append(p.string());
	}
	std::cout << cmd << std::endl;
	std::cout << exec(cmd);
	/*
	 * libtool them all
	 */
	std::string libraryFinalFileName;
	libraryFinalFileName.append("objects/lib").append(libraryName).append(".a");
	cmd = "ar -rcT ";
	cmd.append(libraryFinalFileName);
	for (fs::path &p : libraryFiles) {
		cmd.append(" ").append(p.string());
	}
	std::cout << cmd << std::endl;
	std::cout << exec(cmd);

	system("pause");
	return 0;
}
