#pragma once /* DirectoryIndexer.hpp */

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Logger.hpp"

using std::string;

typedef std::vector<std::pair<std::string, std::pair<long, size_t> > > Entries;

class DirectoryIndexer {
  public:
    ~DirectoryIndexer() {}
    DirectoryIndexer(Logger &_log) : log(_log) {}

    string indexDirectory(string location, string path);

  private:
    void iterateOverDirEntries(Entries &entries, struct dirent *&entry,
                               const string &path);

    Logger &log;
};
