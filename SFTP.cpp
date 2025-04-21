#include "SFTP.h"
#include <curl/curl.h>
#include <string>

int UploadFileFTPS(const std::string& ftpUrl, const std::string& ftpUser, const std::string& ftpPassword, const std::string& filePath, const std::string& savePath) {
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        std::string fullUrl = ftpUrl + "/" + savePath;
        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
        curl_easy_setopt(curl, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_TLS);
        curl_easy_setopt(curl, CURLOPT_USERPWD, (ftpUser + ":" + ftpPassword).c_str());
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        FILE* hd_src = fopen(filePath.c_str(), "rb");
        if (hd_src) {
            curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
            res = curl_easy_perform(curl);
            fclose(hd_src);

            if (res != CURLE_OK) {
                curl_easy_cleanup(curl);
                return 1;
            }
        }
        else {
            curl_easy_cleanup(curl);
            return 2;
        }

        curl_easy_cleanup(curl);
    }
    else {
        return 3;
    }

    curl_global_cleanup();
    return 0;
}