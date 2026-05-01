#pragma once
#include <string>
#include <unordered_map>

namespace interfaces {
struct HttpResponse {
    int statusCode;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

class IRestClient {
   public:
    virtual ~IRestClient() = default;

    virtual HttpResponse get(const std::string_view &url,
                             const std::unordered_map<std::string, std::string> &headers = {}) = 0;

    virtual HttpResponse post(const std::string_view &url, const std::string_view &body,
                              const std::unordered_map<std::string, std::string> &headers = {}) = 0;

    virtual HttpResponse put(const std::string_view &url, const std::string_view &body,
                             const std::unordered_map<std::string, std::string> &headers = {}) = 0;

    virtual HttpResponse patch(const std::string_view &url, const std::string_view &body,
                               const std::unordered_map<std::string, std::string> &headers = {}) = 0;

    virtual HttpResponse del(const std::string_view &url,
                             const std::unordered_map<std::string, std::string> &headers = {}) = 0;
};

}  // namespace interfaces