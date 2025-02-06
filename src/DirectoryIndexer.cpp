#include <algorithm>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <utility>

#include "DirectoryIndexer.hpp"
#include "Logger.hpp"
#include "Repr.hpp"
#include "Utils.hpp"

using std::string;

void DirectoryIndexer::iterateOverDirEntries(Entries &entries, struct dirent *&entry,
                                             const string &path) {
    log.trace() << "Entry name: " << repr((char *)entry->d_name) << std::endl;
    if (std::strcmp(entry->d_name, ".") == 0) {
        log.trace() << "Entry name is a single dot, skipping" << std::endl;
        return;
    }
    string entryPath = path + "/" + entry->d_name;
    struct stat st;
    if (::stat(entryPath.c_str(), &st) == -1) {
        log.warn() << "indexDirectory: Failed to stat file " << entryPath << ": "
                   << ::strerror(errno) << std::endl;
        ;
        return;
    }
    string nameField = entry->d_name;
    if (S_ISDIR(st.st_mode)) {
        nameField += "/";
    }
    long lastModified = static_cast<long>(st.st_mtime);
    size_t sizeBytes = static_cast<size_t>(st.st_size);
    log.trace() << "Pushing back directory entry " << repr((char *)entry->d_name)
                << " to vector" << std::endl;
    log.trace() << "Name field (formatted): " << repr(nameField) << std::endl;
    log.trace() << "Last modified (formatted): "
                << repr(Utils::formattedTimestamp(lastModified)) << std::endl;
    log.trace() << "Size in bytes (formatted): " << repr(Utils::formatSI(sizeBytes))
                << std::endl;
    entries.push_back(std::make_pair(nameField, std::make_pair(lastModified, sizeBytes)));
}

string DirectoryIndexer::indexDirectory(string location, string path) {
    log.debug() << "Indexing directory " << repr(path) << " coming from location "
                << repr(location) << std::endl;
    DIR *dir = ::opendir(path.c_str());
    if (!dir) {
        log.warn() << "indexDirectory: Failed to open directory " << path
                   << std::endl; // TODO: @timo: make logging proper
        return "<h1>Couldn't get directory contents</h1>"; // not perfect, should throw
                                                           // 404 or smth
    }

    Entries entries;
    struct dirent *entry;
    log.debug() << "Iterating over directory entry pointer" << std::endl;
    while ((entry = ::readdir(dir)) != NULL) {
        iterateOverDirEntries(entries, entry, path);
    }
    ::closedir(dir);
    std::ostringstream result;
    result << "<h1>Index of " << location << "</h1>\n";
    result << "<table>\n";
    std::sort(entries.begin(), entries.end());
    log.debug() << "Formatting directory entries into an html table" << std::endl;
    for (Entries::iterator it = entries.begin(); it != entries.end(); ++it) {
        result << "<tr>";
        {
            result << "<td>" << "<a href=\"" << location << it->first << "\">"
                   << it->first << "</a>" << "</td>";
            result << "<td>" << Utils::formattedTimestamp(it->second.first) << "<td>";
            result << "<td>" << Utils::formatSI(it->second.second) << "<td>";
        }
        result << "</tr>\n";
    }
    result << "</table>\n";
    log.debug() << "Returning that html table" << std::endl;
    log.trace2() << "Table is: " << repr(result.str()) << std::endl;
    return result.str();
}
