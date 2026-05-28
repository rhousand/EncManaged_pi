#include "downloader.h"
#include <curl/curl.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>
#include <wx/filename.h>
#include <thread>
#include <mutex>
#include <queue>
#include <filesystem>
#include <chrono>
#include <cstdio>

namespace fs = std::filesystem;

// --- ZIP extraction ---

static bool ExtractZip(const std::string& zipPath, const std::string& destDir) {
    wxFFileInputStream in(zipPath);
    if (!in.IsOk()) return false;
    wxZipInputStream zip(in);

    std::unique_ptr<wxZipEntry> entry;
    while ((entry.reset(zip.GetNextEntry()), entry)) {
        std::string name = entry->GetName().ToStdString();
        // zip-slip guard
        if (name.find("..") != std::string::npos) continue;

        fs::path target = fs::path(destDir) / name;

        if (entry->IsDir()) {
            fs::create_directories(target);
            continue;
        }
        fs::create_directories(target.parent_path());
        wxFFileOutputStream out(target.string());
        if (!out.IsOk()) return false;
        zip.Read(out);
    }
    return true;
}

// --- HTTP download ---

static size_t WriteCallback(char* ptr, size_t sz, size_t nmemb, void* ud) {
    auto* f = static_cast<FILE*>(ud);
    return fwrite(ptr, sz, nmemb, f);
}

static bool DownloadFile(const std::string& url, const std::string& dest,
                         std::string& err)
{
    FILE* f = fopen(dest.c_str(), "wb");
    if (!f) { err = "cannot open dest file"; return false; }

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      f);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        300L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode rc = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);
    fclose(f);

    if (rc != CURLE_OK) { err = curl_easy_strerror(rc); return false; }
    if (httpCode != 200) { err = "HTTP " + std::to_string(httpCode); return false; }
    return true;
}

// --- Atomic swap ---

static DownloadResult ProcessJob(const DownloadJob& job) {
    DownloadResult result;
    result.cellName = job.cell.name;

    const std::string& name    = job.cell.name;
    const std::string& destDir = job.destDir;
    fs::path finalDir  = fs::path(destDir) / name;
    fs::path backupDir = fs::path(destDir) / (name + ".old");

    std::string threadTag = std::to_string(
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
    fs::path tempDir = fs::path(destDir) / (".tmp-" + name + "-" + threadTag);

    std::error_code ec;
    fs::create_directories(tempDir, ec);
    if (ec) { result.err = "mktemp: " + ec.message(); return result; }

    auto cleanup = [&]() { fs::remove_all(tempDir); };

    // Download with up to 3 attempts, exponential backoff
    std::string zipPath = (tempDir / (name + ".zip")).string();
    std::string dlErr;
    bool downloaded = false;
    for (int attempt = 0; attempt < 3 && !downloaded; ++attempt) {
        if (attempt > 0)
            std::this_thread::sleep_for(std::chrono::seconds(attempt * attempt));
        downloaded = DownloadFile(job.cell.zipfileLocation, zipPath, dlErr);
    }
    if (!downloaded) {
        cleanup();
        result.err = dlErr;
        return result;
    }

    // Extract
    if (!ExtractZip(zipPath, tempDir.string())) {
        cleanup();
        result.err = "zip extraction failed";
        return result;
    }
    fs::remove(zipPath);

    // Atomic swap
    if (fs::exists(finalDir)) {
        fs::rename(finalDir, backupDir, ec);
        if (ec) { cleanup(); result.err = "backup rename: " + ec.message(); return result; }

        fs::rename(tempDir, finalDir, ec);
        if (ec) {
            fs::rename(backupDir, finalDir);  // restore on failure
            result.err = "swap rename: " + ec.message();
            return result;
        }
        fs::remove_all(backupDir);
    } else {
        fs::rename(tempDir, finalDir, ec);
        if (ec) { cleanup(); result.err = "move: " + ec.message(); return result; }
    }

    result.ok = true;
    return result;
}

// --- Thread pool ---

void RunDownloads(const std::vector<DownloadJob>& jobs,
                  int workers,
                  ResultCallback onResult)
{
    std::queue<DownloadJob> q;
    for (auto& j : jobs) q.push(j);

    std::mutex mu;
    std::vector<std::thread> threads;
    threads.reserve(workers);

    for (int i = 0; i < workers; ++i) {
        threads.emplace_back([&]() {
            while (true) {
                DownloadJob job;
                {
                    std::lock_guard<std::mutex> lock(mu);
                    if (q.empty()) return;
                    job = q.front();
                    q.pop();
                }
                onResult(ProcessJob(job));
            }
        });
    }
    for (auto& t : threads) t.join();
}
