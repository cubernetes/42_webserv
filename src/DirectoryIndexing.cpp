#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <dirent.h>
#include <iomanip>

#include "Logger.hpp"

using std::string;

typedef std::vector<std::pair<std::string, std::pair<long, long long> > > Entries;

static std::string formatSizeReadable(long long size) {
	std::ostringstream oss;

    if (size < 1024) {
		oss << size << " B";
    } else if (size < 1024 * 1024) {
		oss << std::fixed << std::setprecision(2) << (double)size / (1024.0) << " KB";
    } else if (size < 1024 * 1024 * 1024) {
		oss << std::fixed << std::setprecision(2) << (double)size / (1024.0 * 1024.0) << " MB";
    } else {
		oss << std::fixed << std::setprecision(2) << (double)size / (1024.0 * 1024.0 * 1024.0) << " GB";
    }

    return oss.str();
}

static void iterateOverDirEntries(Entries& entries, struct dirent*& entry, const string& path) {
	if (std::strcmp(entry->d_name, ".") == 0)
		return;
	string entryPath = path + "/" + entry->d_name;
	struct stat st;
	if (stat(entryPath.c_str(), &st) == -1) {
		Logger::logError("indexDirectory: Failed to stat file " + entryPath + ": " + std::strerror(errno));
		return;
	}
	string nameField = entry->d_name;
	if (S_ISDIR(st.st_mode)) {
		nameField += "/";
	}
	long lastModified = static_cast<long>(st.st_mtime);
	long long sizeBytes = static_cast<long long>(st.st_size);
	entries.push_back(std::make_pair(nameField, std::make_pair(lastModified, sizeBytes)));
}

string indexDirectory(string location, string path) {
	DIR* dir = opendir(path.c_str());
	if (!dir) {
		Logger::logError("indexDirectory: Failed to open directory " + path); // TODO: @timo: make logging proper
		return "Couldn't get directory contents"; // not perfect, should throw 404 or smth
	}

	Entries entries;
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		iterateOverDirEntries(entries, entry, path);
	}
	closedir(dir);
	std::ostringstream result;
	result << "<h1>Index of " << location << "</h1>\n";
	result << "<table>\n";
	std::sort(entries.begin(), entries.end());
	for (Entries::iterator it = entries.begin(); it != entries.end(); ++it) {
		result << "<tr>";
		{
			result << "<td>" << "<a href=\"" << location << it->first << "\">" << it->first << "</a>" << "</td>";
			// result << "<td>" << it->second.first << "<td>"; // not readable
			result << "<td>" << formatSizeReadable(it->second.second) << "<td>";
		}
		result << "</tr>\n";
	}
	result << "</table>\n";
	return result.str();
}
