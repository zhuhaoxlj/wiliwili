//
// Created by kiro on 2024/12/2.
//

#include "api/nas/nas_config.hpp"
#include <regex>
#include <sstream>
#include <iomanip>

std::string NASConfig::buildUrl(const std::string& path) const {
    if (serverUrl.empty()) return "";

    std::string url = serverUrl;
    // 确保 serverUrl 末尾没有斜杠
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }

    // 确保 path 以斜杠开头
    std::string encodedPath = urlEncode(path);
    if (encodedPath.empty() || encodedPath[0] != '/') {
        url += "/";
    }
    url += encodedPath;

    return url;
}

std::string NASConfig::buildAuthUrl(const std::string& path) const {
    if (serverUrl.empty()) return "";

    // 解析 serverUrl 获取协议和主机部分
    std::string protocol;
    std::string host;

    size_t protocolEnd = serverUrl.find("://");
    if (protocolEnd != std::string::npos) {
        protocol = serverUrl.substr(0, protocolEnd + 3);  // 包含 "://"
        host = serverUrl.substr(protocolEnd + 3);
    } else {
        protocol = "http://";
        host = serverUrl;
    }

    // 移除 host 末尾的斜杠
    if (!host.empty() && host.back() == '/') {
        host.pop_back();
    }

    // 构建带认证的 URL: protocol + username:password@ + host + path
    std::string url = protocol;
    if (!username.empty()) {
        url += urlEncode(username);
        if (!password.empty()) {
            url += ":" + urlEncode(password);
        }
        url += "@";
    }
    url += host;

    // 添加路径
    std::string encodedPath = urlEncode(path);
    if (encodedPath.empty() || encodedPath[0] != '/') {
        url += "/";
    }
    url += encodedPath;

    return url;
}

bool NASConfig::isValidServerAddress(const std::string& address) {
    if (address.empty()) return false;

    std::string addr = address;

    // 移除协议前缀进行验证
    size_t protocolEnd = addr.find("://");
    if (protocolEnd != std::string::npos) {
        std::string protocol = addr.substr(0, protocolEnd);
        // 只支持 http 和 https
        if (protocol != "http" && protocol != "https") {
            return false;
        }
        addr = addr.substr(protocolEnd + 3);
    }

    // 移除末尾斜杠和路径
    size_t pathStart = addr.find('/');
    if (pathStart != std::string::npos) {
        addr = addr.substr(0, pathStart);
    }

    if (addr.empty()) return false;

    // 分离主机和端口
    std::string host;
    std::string port;
    size_t colonPos = addr.rfind(':');
    if (colonPos != std::string::npos) {
        host = addr.substr(0, colonPos);
        port = addr.substr(colonPos + 1);
        // 验证端口号
        if (!port.empty()) {
            for (char c : port) {
                if (!std::isdigit(c)) return false;
            }
            int portNum = std::stoi(port);
            if (portNum < 1 || portNum > 65535) return false;
        }
    } else {
        host = addr;
    }

    if (host.empty()) return false;

    // 验证 IP 地址格式
    std::regex ipRegex(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)");
    std::smatch match;
    if (std::regex_match(host, match, ipRegex)) {
        // 验证每个数字在 0-255 范围内
        for (int i = 1; i <= 4; i++) {
            int num = std::stoi(match[i].str());
            if (num < 0 || num > 255) return false;
        }
        return true;
    }

    // 验证域名格式
    // 域名可以包含字母、数字、连字符和点
    std::regex domainRegex(R"(^[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?)*$)");
    return std::regex_match(host, domainRegex);
}

std::string NASConfig::urlEncode(const std::string& str) {
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
