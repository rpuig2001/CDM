#include "CurlRestClient.h"

using namespace api;

static std::size_t receiveCurlData(void* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    std::string* buffer = static_cast<std::string*>(userdata);
    buffer->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

CurlRestClient::CurlRestClient() : m_getRequest(), m_putRequest(), m_patchRequest(), m_deleteRequest() {
    // Configure GET
    curl_easy_setopt(m_getRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_getRequest.socket, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurlData);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_TIMEOUT, 2L);
    curl_easy_setopt(m_getRequest.socket, CURLOPT_CONNECTTIMEOUT, 30L);

    // Configure PUT
    curl_easy_setopt(m_putRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_putRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurlData);
    curl_easy_setopt(m_putRequest.socket, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(m_putRequest.socket, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(m_putRequest.socket, CURLOPT_CONNECTTIMEOUT, 30L);

    // Configure PATCH
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurlData);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_CONNECTTIMEOUT, 30L);

    // Configure DELETE
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_WRITEFUNCTION, receiveCurlData);
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_TIMEOUT, 2L);
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_CONNECTTIMEOUT, 30L);
}

CurlRestClient::~CurlRestClient() {
    if (nullptr != m_getRequest.socket) {
        std::lock_guard guard(m_getRequest.lock);
        curl_easy_cleanup(m_getRequest.socket);
        m_getRequest.socket = nullptr;
    }

    if (nullptr != m_putRequest.socket) {
        std::lock_guard guard(m_putRequest.lock);
        curl_easy_cleanup(m_putRequest.socket);
        m_putRequest.socket = nullptr;
    }

    if (nullptr != m_patchRequest.socket) {
        std::lock_guard guard(m_patchRequest.lock);
        curl_easy_cleanup(m_patchRequest.socket);
        m_patchRequest.socket = nullptr;
    }

    if (nullptr != m_deleteRequest.socket) {
        std::lock_guard guard(m_deleteRequest.lock);
        curl_easy_cleanup(m_deleteRequest.socket);
        m_deleteRequest.socket = nullptr;
    }
}

interfaces::HttpResponse CurlRestClient::get(const std::string& url,
                                             const std::unordered_map<std::string, std::string>& headers) {
    std::lock_guard guard(this->m_getRequest.lock);
    interfaces::HttpResponse response;

    if (nullptr == this->m_getRequest.socket) {
        response.statusCode = -1;
        response.body = "Internal client error: GET CURL handle not initialized.";
        return response;
    }

    std::string responseBody;

    // apply headers (if provided)
    struct curl_slist* chunk = nullptr;
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        chunk = curl_slist_append(chunk, header.c_str());
    }
    curl_easy_setopt(m_getRequest.socket, CURLOPT_HTTPHEADER, chunk);

    // configure request
    curl_easy_setopt(m_getRequest.socket, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_getRequest.socket, CURLOPT_WRITEDATA, &responseBody);

    // perform request
    CURLcode result = curl_easy_perform(m_getRequest.socket);

    if (CURLE_OK == result) {
        long statusCode = 0;
        curl_easy_getinfo(m_getRequest.socket, CURLINFO_RESPONSE_CODE, &statusCode);

        response.statusCode = static_cast<int>(statusCode);
        response.body = responseBody;
    } else {
        response.statusCode = -1;
        response.body = curl_easy_strerror(result);
    }

    if (chunk) {
        curl_slist_free_all(chunk);
    }

    return response;
}

interfaces::HttpResponse CurlRestClient::post(const std::string& url, const std::string_view& body,
                                              const std::unordered_map<std::string, std::string>& headers) {
    interfaces::HttpResponse response;

    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> socket(curl_easy_init(), curl_easy_cleanup);
    if (!socket) {
        response.statusCode = -1;
        response.body = "Internal client error: POST CURL handle initialization failed.";
        return response;
    }

    curl_easy_setopt(socket.get(), CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
    curl_easy_setopt(socket.get(), CURLOPT_WRITEFUNCTION, receiveCurlData);
    curl_easy_setopt(socket.get(), CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(socket.get(), CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(socket.get(), CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(socket.get(), CURLOPT_NOSIGNAL, 1L);

    std::string responseBody;

    // apply headers (if provided)
    struct curl_slist* chunk = nullptr;
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        chunk = curl_slist_append(chunk, header.c_str());
    }
    curl_easy_setopt(socket.get(), CURLOPT_HTTPHEADER, chunk);

    // configure request
    curl_easy_setopt(socket.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(socket.get(), CURLOPT_POSTFIELDS, body.data());
    curl_easy_setopt(socket.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(socket.get(), CURLOPT_WRITEDATA, &responseBody);

    // perform request
    CURLcode result = curl_easy_perform(socket.get());

    if (CURLE_OK == result) {
        long statusCode = 0;
        curl_easy_getinfo(socket.get(), CURLINFO_RESPONSE_CODE, &statusCode);

        response.statusCode = static_cast<int>(statusCode);
        response.body = responseBody;
    } else {
        response.statusCode = -1;
        response.body = curl_easy_strerror(result);
    }

    if (chunk) {
        curl_slist_free_all(chunk);
    }

    return response;
}

interfaces::HttpResponse CurlRestClient::put(const std::string& url, const std::string_view& body,
                                             const std::unordered_map<std::string, std::string>& headers) {
    std::lock_guard guard(this->m_putRequest.lock);
    interfaces::HttpResponse response;

    if (nullptr == this->m_putRequest.socket) {
        response.statusCode = -1;
        response.body = "Internal client error: PUT CURL handle not initialized.";
        return response;
    }

    std::string responseBody;

    // apply headers (if provided)
    struct curl_slist* chunk = nullptr;
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        chunk = curl_slist_append(chunk, header.c_str());
    }
    curl_easy_setopt(m_putRequest.socket, CURLOPT_HTTPHEADER, chunk);

    // configure request
    curl_easy_setopt(m_putRequest.socket, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_putRequest.socket, CURLOPT_POSTFIELDS, body.data());
    curl_easy_setopt(m_putRequest.socket, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(m_putRequest.socket, CURLOPT_WRITEDATA, &responseBody);

    // perform request
    CURLcode result = curl_easy_perform(m_putRequest.socket);

    if (CURLE_OK == result) {
        long statusCode = 0;
        curl_easy_getinfo(m_putRequest.socket, CURLINFO_RESPONSE_CODE, &statusCode);

        response.statusCode = static_cast<int>(statusCode);
        response.body = responseBody;
    } else {
        response.statusCode = -1;
        response.body = curl_easy_strerror(result);
    }

    if (chunk) {
        curl_slist_free_all(chunk);
    }

    return response;
}

interfaces::HttpResponse CurlRestClient::patch(const std::string& url, const std::string_view& body,
                                               const std::unordered_map<std::string, std::string>& headers) {
    std::lock_guard guard(this->m_patchRequest.lock);
    interfaces::HttpResponse response;

    if (nullptr == this->m_patchRequest.socket) {
        response.statusCode = -1;
        response.body = "Internal client error: PATCH CURL handle not initialized.";
        return response;
    }

    std::string responseBody;

    // apply headers
    struct curl_slist* chunk = nullptr;
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        chunk = curl_slist_append(chunk, header.c_str());
    }
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_HTTPHEADER, chunk);

    // configure request
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_POSTFIELDS, body.data());
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(m_patchRequest.socket, CURLOPT_WRITEDATA, &responseBody);

    // perform request
    CURLcode result = curl_easy_perform(m_patchRequest.socket);

    if (CURLE_OK == result) {
        long statusCode = 0;
        curl_easy_getinfo(m_patchRequest.socket, CURLINFO_RESPONSE_CODE, &statusCode);

        response.statusCode = static_cast<int>(statusCode);
        response.body = responseBody;
    } else {
        response.statusCode = -1;
        response.body = curl_easy_strerror(result);
    }

    if (chunk) {
        curl_slist_free_all(chunk);
    }

    return response;
}

interfaces::HttpResponse CurlRestClient::del(const std::string& url,
                                             const std::unordered_map<std::string, std::string>& headers) {
    std::lock_guard guard(this->m_deleteRequest.lock);
    interfaces::HttpResponse response;

    if (nullptr == this->m_deleteRequest.socket) {
        response.statusCode = -1;
        response.body = "Internal client error: DELETE CURL handle not initialized.";
        return response;
    }

    std::string responseBody;

    // apply headers
    struct curl_slist* chunk = nullptr;
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        chunk = curl_slist_append(chunk, header.c_str());
    }
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_HTTPHEADER, chunk);

    // configure request
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_deleteRequest.socket, CURLOPT_WRITEDATA, &responseBody);

    // perform request
    CURLcode result = curl_easy_perform(m_deleteRequest.socket);

    if (CURLE_OK == result) {
        long statusCode = 0;
        curl_easy_getinfo(m_deleteRequest.socket, CURLINFO_RESPONSE_CODE, &statusCode);

        response.statusCode = static_cast<int>(statusCode);
        response.body = responseBody;
    } else {
        response.statusCode = -1;
        response.body = curl_easy_strerror(result);
    }

    if (chunk) {
        curl_slist_free_all(chunk);
    }

    return response;
}