#include "catalog.h"
#include "tinyxml2/tinyxml2.h"
#include <curl/curl.h>
#include <string>

static size_t CurlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

bool FetchCatalog(const std::string& url,
                  std::vector<CellInfo>& out,
                  std::string& err)
{
    std::string body;
    CURL* curl = curl_easy_init();
    if (!curl) { err = "curl_easy_init failed"; return false; }

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        60L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode rc = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        err = std::string("curl: ") + curl_easy_strerror(rc);
        return false;
    }
    if (httpCode != 200) {
        err = "HTTP " + std::to_string(httpCode);
        return false;
    }

    tinyxml2::XMLDocument doc;
    if (doc.Parse(body.c_str()) != tinyxml2::XML_SUCCESS) {
        err = "XML parse failed";
        return false;
    }

    auto* root = doc.RootElement();
    if (!root) { err = "empty XML"; return false; }

    for (auto* node = root->FirstChildElement("cell");
         node;
         node = node->NextSiblingElement("cell"))
    {
        CellInfo c;
        auto get = [&](const char* tag) -> std::string {
            auto* el = node->FirstChildElement(tag);
            return (el && el->GetText()) ? el->GetText() : "";
        };

        c.name            = get("name");
        c.status          = get("status");
        c.zipfileLocation = get("zipfile_location");

        auto edtnStr  = get("edtn");
        auto updnStr  = get("updn");
        auto sizeStr  = get("zipfile_size");

        c.edtn        = edtnStr.empty()  ? 0 : std::stoi(edtnStr);
        c.updn        = updnStr.empty()  ? 0 : std::stoi(updnStr);
        c.zipfileSize = sizeStr.empty()  ? 0 : std::stoll(sizeStr);

        if (!c.name.empty()) out.push_back(std::move(c));
    }
    return true;
}
