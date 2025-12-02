//
// Created by kiro on 2024/12/2.
//

#pragma once

#include <string>
#include <nlohmann/json.hpp>

/**
 * NAS 配置结构体
 * 用于存储飞牛 NAS (FNOS) 的 WebDAV 连接配置
 */
struct NASConfig {
    std::string serverUrl;   // WebDAV 服务器地址，如 http://192.168.1.100:5005
    std::string username;    // 用户名
    std::string password;    // 密码
    std::string lastPath;    // 上次浏览路径
    bool enabled = false;    // 是否已配置

    /**
     * 构建完整的 WebDAV URL
     * @param path 文件路径
     * @return 完整的 URL
     */
    std::string buildUrl(const std::string& path) const;

    /**
     * 构建带认证的 URL（用于 MPV 播放）
     * @param path 文件路径
     * @return 带认证信息的 URL
     */
    std::string buildAuthUrl(const std::string& path) const;

    /**
     * 验证服务器地址格式是否有效
     * 支持 IP 地址、域名、带端口的地址
     * @param address 服务器地址
     * @return 是否有效
     */
    static bool isValidServerAddress(const std::string& address);

    /**
     * URL 编码
     * @param str 需要编码的字符串
     * @return 编码后的字符串
     */
    static std::string urlEncode(const std::string& str);
};

// JSON 序列化
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NASConfig, serverUrl, username, password, lastPath, enabled);
