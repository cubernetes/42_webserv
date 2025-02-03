#pragma once /* DirectoryIndexer.hpp */

#include <string>
#include <utility>
#include <vector>

#include "Logger.hpp"

using std::string;

typedef std::vector<std::pair<std::string, std::pair<long, long long> > > Entries;

class DirectoryIndexer {
  public:
    ~DirectoryIndexer() {}
    DirectoryIndexer(Logger &_log) : log(_log) {}

    string indexDirectory(string location, string path);

  private:
    void iterateOverDirEntries(Entries &entries, struct dirent *&entry, const string &path);
    std::string formatSizeReadable(long long size);

    Logger &log;
};
