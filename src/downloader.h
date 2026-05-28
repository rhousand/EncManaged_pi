#pragma once
#include "catalog.h"
#include <string>
#include <vector>
#include <functional>

struct DownloadJob {
    CellInfo    cell;
    std::string destDir;
};

struct DownloadResult {
    std::string cellName;
    bool        ok{false};
    std::string err;
};

using ResultCallback = std::function<void(const DownloadResult&)>;

// Runs jobs with `workers` parallel threads.
// onResult is called from worker threads — must be thread-safe.
void RunDownloads(const std::vector<DownloadJob>& jobs,
                  int workers,
                  ResultCallback onResult);
