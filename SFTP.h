#ifndef FTP_UPLOAD_H
#define FTP_UPLOAD_H

#include <string>

int UploadFileFTPS(const std::string& ftpUrl, const std::string& ftpUser, const std::string& ftpPassword, const std::string& filePath, const std::string& savePath);

#endif // FTP_UPLOAD_H