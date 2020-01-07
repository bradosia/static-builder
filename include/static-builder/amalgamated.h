#include <string>
#include <filesystem>
#include <errno.h>

std::string exec(const char *cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		throw std::runtime_error(std::string("popen() failed: ").append(strerror(errno)));
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}

std::string exec(std::string cmdString) {
	const char *cmd = cmdString.c_str();
	return exec(cmd);
}

std::filesystem::path removeParentDir(std::filesystem::path path) {
	std::string pathString = path.string();
	unsigned int len = pathString.length();
	if (len > 2) {
		path = pathString.substr(2, len - 2);
	}
	return path;
}
