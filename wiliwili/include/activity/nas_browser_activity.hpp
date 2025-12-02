//
// Created by kiro on 2024/12/2.
//

#pragma once

#include <borealis/core/activity.hpp>
#include <borealis/core/bind.hpp>
#include <borealis/views/recycler.hpp>
#include <memory>
#include <vector>

#include "api/nas/webdav_client.hpp"

namespace brls {
class Label;
class Box;
class ProgressSpinner;
class RecyclerFrame;
}  // namespace brls

class WebDAVClient;

/**
 * NAS 文件浏览 Activity
 * 用于浏览飞牛 NAS (FNOS) 上的文件和文件夹
 */
class NASBrowserActivity : public brls::Activity {
public:
    CONTENT_FROM_XML_RES("activity/nas_browser_activity.xml");

    /**
     * 构造函数
     * @param initialPath 初始路径，默认为根目录
     */
    explicit NASBrowserActivity(const std::string& initialPath = "/");

    void onContentAvailable() override;

    ~NASBrowserActivity() override;

    /**
     * 处理文件/文件夹选择
     * @param item 选中的项
     */
    void onItemSelected(const WebDAVItem& item);

private:
    BRLS_BIND(brls::RecyclerFrame, fileList, "nas/file/list");
    BRLS_BIND(brls::Label, pathLabel, "nas/path/label");
    BRLS_BIND(brls::Label, statusLabel, "nas/status/label");
    BRLS_BIND(brls::Box, loadingBox, "nas/loading/box");

    std::string currentPath;
    std::string initialPath;
    std::unique_ptr<WebDAVClient> webdavClient;
    std::vector<WebDAVItem> currentItems;
    
    // 用于检测 Activity 是否还存活，防止异步回调访问已释放的内存
    std::shared_ptr<bool> activityAlive = std::make_shared<bool>(true);

    /**
     * 加载目录内容
     * @param path 目录路径
     */
    void loadDirectory(const std::string& path);

    /**
     * 返回上级目录
     */
    void navigateUp();

    /**
     * 显示加载状态
     * @param show 是否显示
     */
    void showLoading(bool show);

    /**
     * 显示状态消息
     * @param message 消息内容
     */
    void showStatus(const std::string& message);

    /**
     * 保存当前路径到配置
     */
    void saveCurrentPath();

    /**
     * 播放视频文件
     * @param item 视频文件项
     */
    void playVideo(const WebDAVItem& item);

    /**
     * 显示错误对话框
     * @param message 错误消息
     * @param showRetry 是否显示重试按钮
     * @param showConfig 是否显示前往配置按钮
     */
    void showErrorDialog(const std::string& message, bool showRetry = true, bool showConfig = false);

    /**
     * 处理网络错误
     * @param error 错误消息
     * @param code HTTP 状态码
     * @param path 请求的路径
     */
    void handleNetworkError(const std::string& error, int code, const std::string& path);
};

/**
 * NAS 文件列表数据源
 */
class NASFileDataSource : public brls::RecyclerDataSource {
public:
    explicit NASFileDataSource(NASBrowserActivity* activity, std::vector<WebDAVItem>* items);

    int numberOfRows(brls::RecyclerFrame* recycler, int section) override;
    brls::RecyclerCell* cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override;
    void didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath index) override;

private:
    NASBrowserActivity* activity;
    std::vector<WebDAVItem>* items;
};

/**
 * NAS 文件列表项单元格
 */
class NASFileCell : public brls::RecyclerCell {
public:
    NASFileCell();

    void setItem(const WebDAVItem& item);

    static NASFileCell* create();

private:
    brls::Label* nameLabel = nullptr;
    brls::Label* sizeLabel = nullptr;
    brls::Label* arrowLabel = nullptr;
    brls::Box* iconBox = nullptr;
};
