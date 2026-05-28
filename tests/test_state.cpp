#include "../src/state.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

int main() {
    std::string dir = "/tmp/enc_state_test";
    fs::remove_all(dir);
    fs::create_directories(dir);

    // Round-trip: save one cell, reload, verify
    {
        SyncState s = LoadState(dir);
        assert(s.cells.empty());

        bool ok = SaveOne(dir, s, "US5WA20M", 3, 7);
        assert(ok);
        assert(s.cells.count("US5WA20M"));
        assert(s.cells["US5WA20M"].edtn == 3);
        assert(s.cells["US5WA20M"].updn == 7);
        std::cout << "SaveOne: PASS\n";
    }
    {
        SyncState s2 = LoadState(dir);
        assert(s2.cells.count("US5WA20M"));
        assert(s2.cells["US5WA20M"].edtn == 3);
        assert(s2.cells["US5WA20M"].updn == 7);
        assert(s2.cells["US5WA20M"].downloadedAt > 0);
        std::cout << "Reload: PASS\n";
    }

    // WriteChtdldr
    {
        SyncState s = LoadState(dir);
        s.catalogFetchedAt = 1700000000;
        bool ok = WriteChtdldr(dir, s);
        assert(ok);
        assert(fs::exists(dir + "/chartdldr_pi.dat"));
        std::cout << "WriteChtdldr: PASS\n";
    }

    // Concurrent SaveOne from two "threads"
    {
        SyncState s = LoadState(dir);
        std::thread t1([&]() { SaveOne(dir, s, "US5MA17M", 1, 0); });
        std::thread t2([&]() { SaveOne(dir, s, "US5NY12M", 2, 1); });
        t1.join();
        t2.join();
        SyncState s3 = LoadState(dir);
        assert(s3.cells.count("US5MA17M"));
        assert(s3.cells.count("US5NY12M"));
        std::cout << "Concurrent SaveOne: PASS\n";
    }

    fs::remove_all(dir);
    std::cout << "All state tests PASS\n";
    return 0;
}
