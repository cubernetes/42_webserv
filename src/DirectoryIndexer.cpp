#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <utility>

#include "DirectoryIndexer.hpp"
#include "Logger.hpp"

using std::string;

std::string DirectoryIndexer::formatSizeReadable(long long size) {
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

void DirectoryIndexer::iterateOverDirEntries(Entries &entries, struct dirent *&entry, const string &path) {
  if (std::strcmp(entry->d_name, ".") == 0)
    return;
  string entryPath = path + "/" + entry->d_name;
  struct stat st;
  if (stat(entryPath.c_str(), &st) == -1) {
    log.debug() << "indexDirectory: Failed to stat file " << entryPath << ": " << std::strerror(errno) << std::endl;
    ;
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

string DirectoryIndexer::indexDirectory(string location, string path) {
  DIR *dir = opendir(path.c_str());
  if (!dir) {
    log.debug() << "indexDirectory: Failed to open directory " << path << std::endl; // TODO: @timo: make logging proper
    return "Couldn't get directory contents"; // not perfect, should throw 404 or smth
  }

  Entries entries;
  struct dirent *entry;
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
      result << "<td>"
             << "<a href=\"" << location << it->first << "\">" << it->first << "</a>"
             << "</td>";
      // result << "<td>" << it->second.first << "<td>"; // not readable
      result << "<td>" << formatSizeReadable(it->second.second) << "<td>";
    }
    result << "</tr>\n";
  }
  result << "</table>\n";
  return result.str();
}
