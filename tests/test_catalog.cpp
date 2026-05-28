#include "../src/catalog.h"
#include <iostream>
#include <cassert>

int main() {
    std::vector<CellInfo> cells;
    std::string err;

    std::cout << "Fetching NOAA ENC catalog...\n";
    bool ok = FetchCatalog("https://www.charts.noaa.gov/ENCs/ENCProdCat.xml", cells, err);

    if (!ok) {
        std::cerr << "FAIL: " << err << "\n";
        return 1;
    }

    std::cout << "Cells: " << cells.size() << "\n";
    assert(!cells.empty());

    int active = 0, cancelled = 0;
    for (auto& c : cells) {
        if (c.status == "Active")    ++active;
        if (c.status == "Cancelled") ++cancelled;
        assert(!c.name.empty());
        assert(!c.zipfileLocation.empty() || c.status == "Cancelled");
    }

    std::cout << "Active: " << active << "  Cancelled: " << cancelled << "\n";

    // Spot-check first active cell
    for (auto& c : cells) {
        if (c.status == "Active") {
            std::cout << "Sample: " << c.name
                      << " edtn=" << c.edtn << " updn=" << c.updn
                      << " size=" << c.zipfileSize << "\n";
            break;
        }
    }

    std::cout << "PASS\n";
    return 0;
}
