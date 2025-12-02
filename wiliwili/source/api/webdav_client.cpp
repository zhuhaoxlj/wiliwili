//
// Created by kiro on 2024/12/2.
//

#include "api/nas/webdav_client.hpp"
#include <tinyxml2.h>
#include <curl/curl.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <borealis/core/thread.hpp>
#include <borealis/core/logger.hpp>

// 支持的视频格式
static const std::vector<std::string> SUPPORTED_VIDEO_EXTENSIONS = {
    ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm",
    ".m4v", ".ts", ".m2ts", ".rmvb", ".rm", ".3gp", ".mpg", ".mpeg"
};

// libcurl 写入回调函数
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

WebDAVClient::WebDAVClient(const NASConfig& config) : config(config) {}

void WebDAVClient::listDirectory(
    const std::string& path,
    const SuccessCallback& callback,
    const ErrorCallback& error
) {
    std::string url = config.buildUrl(path);
    if (url.empty()) {
        if (error) error("Invalid server URL", -1);
        return;
    }

    // 确保路径以 / 结尾（目录）
    if (!url.empty() && url.back() != '/') {
        url += '/';
    }

    // PROPFIND 请求体
    std::string propfindBody = R"(<?xml version="1.0" encoding="utf-8"?>
<propfind xmlns="DAV:">
  <prop>
    <displayname/>
    <getcontentlength/>
    <getcontenttype/>
    <getlastmodified/>
    <resourcetype/>
  </prop>
</propfind>)";

    std::string authHeader = buildAuthHeader();
    std::string basePath = path;
    
    // 使用 borealis 的异步机制执行 PROPFIND 请求
    brls::async([url, propfindBody, authHeader, callback, error, basePath, this]() {
        CURL* curl = curl_easy_init();
        if (!curl) {
            brls::sync([error]() {
                if (error) error("Failed to initialize CURL", -1);
            });
            return;
        }
        
        std::string responseBody;
        long httpCode = 0;
        
        // 设置 URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        // 设置 PROPFIND 方法
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PROPFIND");
        
        // 设置请求体
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, propfindBody.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, propfindBody.size());
        
        // 设置请求头
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/xml");
        headers = curl_slist_append(headers, "Depth: 1");
        if (!authHeader.empty()) {
            std::string authHeaderStr = "Authorization: Basic " + authHeader;
            headers = curl_slist_append(headers, authHeaderStr.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        // 设置写入回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
        
        // 设置超时
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        // 禁用 SSL 验证
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        // 禁用代理（NAS 通常在内网，不需要走代理）
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        
        // 执行请求
        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        
        // 清理
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        // 在主线程中处理回调
        brls::sync([res, httpCode, responseBody, callback, error, basePath, this]() {
            if (res != CURLE_OK) {
                std::string errorMsg = curl_easy_strerror(res);
                if (error) error(errorMsg, -1);
                return;
            }

            if (httpCode == 401) {
                if (error) error("Authentication failed", 401);
                return;
            }

            if (httpCode == 404) {
                if (error) error("Path not found", 404);
                return;
            }

            // WebDAV PROPFIND 成功返回 207 Multi-Status
            if (httpCode != 207 && httpCode != 200) {
                if (error) error("Server error: " + std::to_string(httpCode), httpCode);
                return;
            }

            auto items = parsePropfindResponse(responseBody, basePath);
            if (callback) callback(items);
        });
    });
}

void WebDAVClient::checkConnection(
    const ConnectionCallback& success,
    const ErrorCallback& error
) {
    std::string url = config.buildUrl("/");
    if (url.empty()) {
        if (error) error("Invalid server URL", -1);
        return;
    }

    // 简单的 PROPFIND 请求检查连接
    std::string propfindBody = R"(<?xml version="1.0" encoding="utf-8"?>
<propfind xmlns="DAV:">
  <prop>
    <resourcetype/>
  </prop>
</propfind>)";

    std::string authHeader = buildAuthHeader();

    // 使用 borealis 的异步机制执行 PROPFIND 请求
    brls::async([url, propfindBody, authHeader, success, error]() {
        CURL* curl = curl_easy_init();
        if (!curl) {
            brls::sync([error]() {
                if (error) error("Failed to initialize CURL", -1);
            });
            return;
        }
        
        std::string responseBody;
        long httpCode = 0;
        
        // 设置 URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        // 设置 PROPFIND 方法
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PROPFIND");
        
        // 设置请求体
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, propfindBody.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, propfindBody.size());
        
        // 设置请求头
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/xml");
        headers = curl_slist_append(headers, "Depth: 0");
        if (!authHeader.empty()) {
            std::string authHeaderStr = "Authorization: Basic " + authHeader;
            headers = curl_slist_append(headers, authHeaderStr.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        // 设置写入回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
        
        // 设置超时
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        // 禁用 SSL 验证
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        // 禁用代理（NAS 通常在内网，不需要走代理）
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        
        // 执行请求
        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        
        // 清理
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        // 在主线程中处理回调
        brls::sync([res, httpCode, success, error]() {
            if (res != CURLE_OK) {
                std::string errorMsg = curl_easy_strerror(res);
                if (error) error(errorMsg, -1);
                return;
            }

            if (httpCode == 401) {
                if (error) error("Authentication failed", 401);
                return;
            }

            if (httpCode == 404) {
                if (error) error("Path not found", 404);
                return;
            }

            if (httpCode != 207 && httpCode != 200) {
                if (error) error("Server error: " + std::to_string(httpCode), httpCode);
                return;
            }

            if (success) success();
        });
    });
}


std::vector<WebDAVItem> WebDAVClient::parsePropfindResponse(const std::string& xml, const std::string& basePath) {
    std::vector<WebDAVItem> items;

    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml.c_str()) != tinyxml2::XML_SUCCESS) {
        return items;
    }

    // 查找 multistatus 根元素
    tinyxml2::XMLElement* multistatus = doc.RootElement();
    if (!multistatus) {
        return items;
    }

    // 规范化基础路径用于比较
    std::string normalizedBasePath = basePath;
    if (!normalizedBasePath.empty() && normalizedBasePath.back() != '/') {
        normalizedBasePath += '/';
    }

    // 遍历所有 response 元素
    for (tinyxml2::XMLElement* response = multistatus->FirstChildElement();
         response != nullptr;
         response = response->NextSiblingElement()) {

        // 检查是否是 response 元素（可能有命名空间前缀）
        std::string elemName = response->Name();
        if (elemName.find("response") == std::string::npos) {
            continue;
        }

        WebDAVItem item;

        // 获取 href
        tinyxml2::XMLElement* href = nullptr;
        for (tinyxml2::XMLElement* child = response->FirstChildElement();
             child != nullptr;
             child = child->NextSiblingElement()) {
            std::string childName = child->Name();
            if (childName.find("href") != std::string::npos) {
                href = child;
                break;
            }
        }

        if (href && href->GetText()) {
            item.path = urlDecode(href->GetText());
            // 移除末尾斜杠以便提取文件名
            std::string pathForName = item.path;
            if (!pathForName.empty() && pathForName.back() == '/') {
                pathForName.pop_back();
            }
            item.name = getFileName(pathForName);
        }

        // 跳过当前目录本身
        std::string itemPathNormalized = item.path;
        if (!itemPathNormalized.empty() && itemPathNormalized.back() != '/') {
            itemPathNormalized += '/';
        }
        // 如果路径与基础路径相同，跳过
        if (item.path == basePath || item.path == normalizedBasePath ||
            itemPathNormalized == normalizedBasePath) {
            continue;
        }

        // 查找 propstat -> prop
        tinyxml2::XMLElement* propstat = nullptr;
        for (tinyxml2::XMLElement* child = response->FirstChildElement();
             child != nullptr;
             child = child->NextSiblingElement()) {
            std::string childName = child->Name();
            if (childName.find("propstat") != std::string::npos) {
                propstat = child;
                break;
            }
        }

        if (propstat) {
            tinyxml2::XMLElement* prop = nullptr;
            for (tinyxml2::XMLElement* child = propstat->FirstChildElement();
                 child != nullptr;
                 child = child->NextSiblingElement()) {
                std::string childName = child->Name();
                if (childName.find("prop") != std::string::npos &&
                    childName.find("propstat") == std::string::npos) {
                    prop = child;
                    break;
                }
            }

            if (prop) {
                // 解析属性
                for (tinyxml2::XMLElement* propChild = prop->FirstChildElement();
                     propChild != nullptr;
                     propChild = propChild->NextSiblingElement()) {

                    std::string propName = propChild->Name();

                    if (propName.find("displayname") != std::string::npos) {
                        if (propChild->GetText()) {
                            item.name = propChild->GetText();
                        }
                    }
                    else if (propName.find("getcontentlength") != std::string::npos) {
                        if (propChild->GetText()) {
                            try {
                                item.size = std::stoll(propChild->GetText());
                            } catch (...) {
                                item.size = 0;
                            }
                        }
                    }
                    else if (propName.find("getcontenttype") != std::string::npos) {
                        if (propChild->GetText()) {
                            item.contentType = propChild->GetText();
                        }
                    }
                    else if (propName.find("getlastmodified") != std::string::npos) {
                        if (propChild->GetText()) {
                            item.lastModified = propChild->GetText();
                        }
                    }
                    else if (propName.find("resourcetype") != std::string::npos) {
                        // 检查是否包含 collection 子元素（表示目录）
                        for (tinyxml2::XMLElement* rtChild = propChild->FirstChildElement();
                             rtChild != nullptr;
                             rtChild = rtChild->NextSiblingElement()) {
                            std::string rtName = rtChild->Name();
                            if (rtName.find("collection") != std::string::npos) {
                                item.isDirectory = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // 只添加有效的项
        if (!item.name.empty()) {
            items.push_back(item);
        }
    }

    // 排序：目录在前，文件在后，同类型按名称排序
    std::sort(items.begin(), items.end(), [](const WebDAVItem& a, const WebDAVItem& b) {
        if (a.isDirectory != b.isDirectory) {
            return a.isDirectory > b.isDirectory;
        }
        return a.name < b.name;
    });

    return items;
}


bool WebDAVClient::isVideoFile(const std::string& filename) {
    if (filename.empty()) return false;

    // 找到最后一个点的位置
    size_t dotPos = filename.rfind('.');
    if (dotPos == std::string::npos || dotPos == filename.length() - 1) {
        return false;
    }

    // 提取扩展名并转换为小写
    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // 检查是否在支持的格式列表中
    for (const auto& supported : SUPPORTED_VIDEO_EXTENSIONS) {
        if (ext == supported) {
            return true;
        }
    }

    return false;
}

std::string WebDAVClient::urlEncode(const std::string& str) {
    std::ostringstream encoded;
    encoded.fill('0');
    encoded << std::hex;

    for (unsigned char c : str) {
        // 保留字母、数字和一些特殊字符
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            encoded << c;
        } else {
            encoded << '%' << std::setw(2) << std::uppercase << static_cast<int>(c);
        }
    }

    return encoded.str();
}

std::string WebDAVClient::urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            // 解析两位十六进制数
            std::string hex = str.substr(i + 1, 2);
            try {
                int value = std::stoi(hex, nullptr, 16);
                result += static_cast<char>(value);
                i += 2;
            } catch (...) {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

std::string WebDAVClient::getParentPath(const std::string& path) {
    if (path.empty() || path == "/") {
        return "/";
    }

    std::string p = path;
    // 移除末尾斜杠
    if (p.back() == '/') {
        p.pop_back();
    }

    // 找到最后一个斜杠
    size_t lastSlash = p.rfind('/');
    if (lastSlash == std::string::npos || lastSlash == 0) {
        return "/";
    }

    return p.substr(0, lastSlash);
}

std::string WebDAVClient::getFileName(const std::string& path) {
    if (path.empty()) return "";

    std::string p = path;
    // 移除末尾斜杠
    if (p.back() == '/') {
        p.pop_back();
    }

    // 找到最后一个斜杠
    size_t lastSlash = p.rfind('/');
    if (lastSlash == std::string::npos) {
        return p;
    }

    return p.substr(lastSlash + 1);
}

std::string WebDAVClient::buildAuthHeader() const {
    if (config.username.empty()) {
        return "";
    }

    std::string credentials = config.username + ":" + config.password;

    // Base64 编码
    static const char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string encoded;
    int val = 0;
    int valb = -6;

    for (unsigned char c : credentials) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (encoded.size() % 4) {
        encoded.push_back('=');
    }

    return encoded;
}
