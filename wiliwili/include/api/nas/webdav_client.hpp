//
// Created by kiro on 2024/12/2.
//

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "api/nas/nas_config.hpp"

/**
 * WebDAV 文件/文件夹项
 */
class WebDAVItem {
public:
    std::string name;           // 文件/文件夹名
    std::string path;           // 完整路径
    bool isDirectory = false;   // 是否为目录
    int64_t size = 0;           // 文件大小（字节）
    std::string contentType;    // MIME 类型
    std::string lastModified;   // 最后修改时间

    WebDAVItem() = default;
    WebDAVItem(const std::string& name, const std::string& path, bool isDir)
        : name(name), path(path), isDirectory(isDir) {}
};

/**
 * WebDAV 客户端
 * 用于访问飞牛 NAS 的 WebDAV 服务
 */
class WebDAVClient {
public:
    using SuccessCallback = std::function<void(std::vector<WebDAVItem>)>;
    using ErrorCallback = std::function<void(const std::string&, int)>;
    using ConnectionCallback = std::function<void()>;

    /**
     * 构造函数
     * @param config NAS 配置
     */
    explicit WebDAVClient(const NASConfig& config);

    /**
     * 列出目录内容
     * @param path 目录路径
     * @param callback 成功回调，返回文件列表
     * @param error 错误回调
     */
    void listDirectory(
        const std::string& path,
        const SuccessCallback& callback,
        const ErrorCallback& error = nullptr
    );

    /**
     * 检查连接是否有效
     * @param success 成功回调
     * @param error 错误回调
     */
    void checkConnection(
        const ConnectionCallback& success,
        const ErrorCallback& error = nullptr
    );

    /**
     * 判断文件是否为视频文件
     * @param filename 文件名
     * @return 是否为视频文件
     */
    static bool isVideoFile(const std::string& filename);

    /**
     * URL 编码
     * @param str 需要编码的字符串
     * @return 编码后的字符串
     */
    static std::string urlEncode(const std::string& str);

    /**
     * 计算父目录路径
     * @param path 当前路径
     * @return 父目录路径
     */
    static std::string getParentPath(const std::string& path);

    /**
     * 从路径中提取文件名
     * @param path 文件路径
     * @return 文件名
     */
    static std::string getFileName(const std::string& path);

private:
    NASConfig config;

    /**
     * 解析 PROPFIND XML 响应
     * @param xml XML 响应内容
     * @param basePath 基础路径（用于过滤当前目录本身）
     * @return 文件列表
     */
    std::vector<WebDAVItem> parsePropfindResponse(const std::string& xml, const std::string& basePath);

    /**
     * URL 解码
     * @param str 需要解码的字符串
     * @return 解码后的字符串
     */
    static std::string urlDecode(const std::string& str);

    /**
     * 构建 Basic Auth 头
     * @return Base64 编码的认证字符串
     */
    std::string buildAuthHeader() const;
};
