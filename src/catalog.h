#pragma once
#include <string>
#include <vector>

struct CellInfo {
    std::string name;
    std::string status;           // "Active" or "Cancelled"
    std::string zipfileLocation;
    int64_t     zipfileSize{0};
    int         edtn{0};
    int         updn{0};
};

// Fetches and parses NOAA ENC catalog XML.
// Returns false and sets err on failure.
bool FetchCatalog(const std::string& url,
                  std::vector<CellInfo>& out,
                  std::string& err);
