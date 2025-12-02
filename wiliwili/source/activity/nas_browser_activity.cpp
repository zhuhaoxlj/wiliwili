//
// Created by kiro on 2024/12/2.
//

#include <borealis/core/i18n.hpp>
#include <borealis/core/application.hpp>
#include <borealis/core/thread.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/recycler.hpp>
#include <borealis/views/progress_spinner.hpp>
#include <borealis/views/dialog.hpp>

#include "activity/nas_browser_activity.hpp"
#include "api/nas/nas_config.hpp"
#include "api/nas/webdav_client.hpp"
#include "utils/config_helper.hpp"
#include "utils/activity_helper.hpp"
#include "utils/dialog_helper.hpp"
#include "view/button_close.hpp"

using namespace brls::literals;

// ============== NASFileCell Implementation ==============

NASFileCell::NASFileCell() {
    this->inflateFromXMLRes("xml/views/nas_file_item.xml");

    nameLabel = dynamic_cast<brls::Label*>(this->getView("nas/item/name"));
    sizeLabel = dynamic_cast<brls::Label*>(this->getView("nas/item/size"));
    arrowLabel = dynamic_cast<brls::Label*>(this->getView("nas/item/arrow"));
}

void NASFileCell::setItem(const WebDAVItem& item) {
    if (nameLabel) {
        nameLabel->setText(item.name);
    }

    if (sizeLabel) {
        if (item.isDirectory) {
            sizeLabel->setText("wiliwili/nas/browser/folder"_i18n);
        } else {
            // Format file size
            std::string sizeStr;
            if (item.size < 1024) {
                sizeStr = std::to_string(item.size) + " B";
            } else if (item.size < 1024 * 1024) {
                sizeStr = std::to_string(item.size / 1024) + " KB";
            } else if (item.size < 1024 * 1024 * 1024) {
                sizeStr = std::to_string(item.size / (1024 * 1024)) + " MB";
            } else {
                sizeStr = std::to_string(item.size / (1024 * 1024 * 1024)) + " GB";
            }
            sizeStr += " - " + (WebDAVClient::isVideoFile(item.name) ? 
                "wiliwili/nas/browser/video"_i18n : "wiliwili/nas/browser/file"_i18n);
            sizeLabel->setText(sizeStr);
        }
    }

    if (arrowLabel) {
        arrowLabel->setVisibility(item.isDirectory ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }
}

NASFileCell* NASFileCell::create() {
    return new NASFileCell();
}


// ============== NASFileDataSource Implementation ==============

NASFileDataSource::NASFileDataSource(NASBrowserActivity* activity, std::vector<WebDAVItem>* items)
    : activity(activity), items(items) {}

int NASFileDataSource::numberOfRows(brls::RecyclerFrame* recycler, int section) {
    return static_cast<int>(items->size());
}

brls::RecyclerCell* NASFileDataSource::cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) {
    NASFileCell* cell = dynamic_cast<NASFileCell*>(recycler->dequeueReusableCell("NASFileCell"));
    if (index.row < static_cast<int>(items->size())) {
        cell->setItem((*items)[index.row]);
    }
    return cell;
}

void NASFileDataSource::didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath index) {
    if (index.row < static_cast<int>(items->size())) {
        activity->onItemSelected((*items)[index.row]);
    }
}

// ============== NASBrowserActivity Implementation ==============

NASBrowserActivity::NASBrowserActivity(const std::string& initialPath)
    : initialPath(initialPath), currentPath(initialPath) {
    brls::Logger::debug("NASBrowserActivity: create with path: {}", initialPath);
}

void NASBrowserActivity::onContentAvailable() {
    brls::Logger::debug("NASBrowserActivity: onContentAvailable");

    // Get NAS config
    auto& conf = ProgramConfig::instance();
    NASConfig nasConfig = conf.getNASConfig();

    if (!nasConfig.enabled || nasConfig.serverUrl.empty()) {
        brls::Logger::error("NASBrowserActivity: NAS not configured");
        showStatus("wiliwili/nas/error/not_configured"_i18n);
        return;
    }

    // Create WebDAV client
    webdavClient = std::make_unique<WebDAVClient>(nasConfig);

    // Register cell type
    fileList->registerCell("NASFileCell", []() { return NASFileCell::create(); });

    // Set data source
    fileList->setDataSource(new NASFileDataSource(this, &currentItems));

    // Register back button handler
    this->registerAction(
        "hints/back"_i18n, brls::ControllerButton::BUTTON_B,
        [this](brls::View* view) -> bool {
            if (currentPath == "/" || currentPath.empty()) {
                // At root, exit activity
                brls::Application::popActivity();
            } else {
                // Navigate up
                navigateUp();
            }
            return true;
        },
        true
    );

    // Use initial path or last path from config
    std::string startPath = initialPath;
    if (startPath == "/" && !nasConfig.lastPath.empty() && nasConfig.lastPath != "/") {
        startPath = nasConfig.lastPath;
    }

    // Load initial directory
    loadDirectory(startPath);
}

void NASBrowserActivity::loadDirectory(const std::string& path) {
    brls::Logger::debug("NASBrowserActivity: loadDirectory: {}", path);

    showLoading(true);
    showStatus("wiliwili/nas/status/loading"_i18n);

    // 使用 shared_ptr 来安全处理异步回调的生命周期
    // 当 Activity 被销毁时，weak_ptr 会失效，回调会检测到并提前返回
    auto weak = std::weak_ptr<bool>(activityAlive);
    
    webdavClient->listDirectory(
        path,
        [weak, this, path](std::vector<WebDAVItem> items) {
            brls::sync([weak, this, path, items = std::move(items)]() mutable {
                // 检查 Activity 是否还存活
                if (weak.expired()) {
                    brls::Logger::debug("NASBrowserActivity: Activity destroyed, ignoring callback");
                    return;
                }
                
                showLoading(false);
                currentPath = path;
                currentItems = std::move(items);

                // Filter to show only directories and video files
                std::vector<WebDAVItem> filteredItems;
                for (const auto& item : currentItems) {
                    if (item.isDirectory || WebDAVClient::isVideoFile(item.name)) {
                        filteredItems.push_back(item);
                    }
                }
                currentItems = std::move(filteredItems);

                // Update UI
                if (pathLabel) {
                    pathLabel->setText(currentPath);
                }

                if (currentItems.empty()) {
                    showStatus("wiliwili/nas/browser/empty"_i18n);
                } else {
                    showStatus(std::to_string(currentItems.size()) + " " + "wiliwili/nas/browser/items"_i18n);
                }

                // Reload data
                fileList->setDataSource(new NASFileDataSource(this, &currentItems));

                // Save current path
                saveCurrentPath();
            });
        },
        [weak, this, path](const std::string& error, int code) {
            brls::sync([weak, this, error, code, path]() {
                // 检查 Activity 是否还存活
                if (weak.expired()) {
                    brls::Logger::debug("NASBrowserActivity: Activity destroyed, ignoring error callback");
                    return;
                }
                
                showLoading(false);
                handleNetworkError(error, code, path);
            });
        }
    );
}

void NASBrowserActivity::onItemSelected(const WebDAVItem& item) {
    brls::Logger::debug("NASBrowserActivity: onItemSelected: {} (isDir: {})", item.name, item.isDirectory);

    if (item.isDirectory) {
        // Navigate into directory
        std::string newPath = item.path;
        // Ensure path doesn't have trailing slash for consistency
        if (!newPath.empty() && newPath.back() == '/') {
            newPath.pop_back();
        }
        loadDirectory(newPath);
    } else if (WebDAVClient::isVideoFile(item.name)) {
        // Play video
        playVideo(item);
    }
}

void NASBrowserActivity::navigateUp() {
    std::string parentPath = WebDAVClient::getParentPath(currentPath);
    brls::Logger::debug("NASBrowserActivity: navigateUp from {} to {}", currentPath, parentPath);
    loadDirectory(parentPath);
}

void NASBrowserActivity::showLoading(bool show) {
    if (loadingBox) {
        loadingBox->setVisibility(show ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }
}

void NASBrowserActivity::showStatus(const std::string& message) {
    if (statusLabel) {
        statusLabel->setText(message);
    }
}

void NASBrowserActivity::saveCurrentPath() {
    auto& conf = ProgramConfig::instance();
    NASConfig nasConfig = conf.getNASConfig();
    nasConfig.lastPath = currentPath;
    conf.setNASConfig(nasConfig);
}

void NASBrowserActivity::playVideo(const WebDAVItem& item) {
    brls::Logger::info("NASBrowserActivity: playVideo: {}", item.name);

    // Get NAS config to build auth URL
    auto& conf = ProgramConfig::instance();
    NASConfig nasConfig = conf.getNASConfig();

    // Build the video URL with authentication
    std::string videoUrl = nasConfig.buildAuthUrl(item.path);
    brls::Logger::debug("NASBrowserActivity: Video URL: {}", videoUrl);

    // Use Intent to play the NAS video
    Intent::playNASVideo(videoUrl, item.name);
}

void NASBrowserActivity::handleNetworkError(const std::string& error, int code, const std::string& path) {
    brls::Logger::error("NASBrowserActivity: Network error - code: {}, error: {}, path: {}", code, error, path);

    std::string errorMsg;
    bool showRetry = true;
    bool showConfig = false;

    switch (code) {
        case 401:
            // Authentication failed
            errorMsg = "wiliwili/nas/error/auth_failed"_i18n;
            showConfig = true;
            break;
        case 404:
            // Path not found
            errorMsg = "wiliwili/nas/error/path_not_found"_i18n;
            // If path not found, try to go to root
            if (path != "/") {
                brls::Logger::warning("NASBrowserActivity: Path not found, going to root");
                showStatus(errorMsg);
                brls::Application::notify(errorMsg);
                loadDirectory("/");
                return;
            }
            break;
        case 500:
        case 502:
        case 503:
        case 504:
            // Server errors
            errorMsg = "wiliwili/nas/error/server_error"_i18n + " (" + std::to_string(code) + ")";
            break;
        case -1:
            // Connection error (timeout, network unreachable, etc.)
            if (error.find("timeout") != std::string::npos || 
                error.find("Timeout") != std::string::npos ||
                error.find("timed out") != std::string::npos) {
                errorMsg = "wiliwili/nas/error/timeout"_i18n;
            } else {
                errorMsg = "wiliwili/nas/error/network_error"_i18n + ": " + error;
            }
            break;
        default:
            // Other errors
            errorMsg = "wiliwili/nas/error/connection_failed"_i18n + ": " + error;
            break;
    }

    showStatus(errorMsg);
    showErrorDialog(errorMsg, showRetry, showConfig);
}

void NASBrowserActivity::showErrorDialog(const std::string& message, bool showRetry, bool showConfig) {
    auto dialog = new brls::Dialog(message);

    if (showRetry) {
        dialog->addButton("wiliwili/nas/error/retry"_i18n, [this]() {
            // Retry loading the current directory
            loadDirectory(currentPath.empty() ? "/" : currentPath);
        });
    }

    if (showConfig) {
        dialog->addButton("wiliwili/nas/error/go_config"_i18n, []() {
            // Go to NAS configuration
            brls::Application::popActivity();
            Intent::openNASConfig();
        });
    }

    // Always add a cancel/back button
    dialog->addButton("hints/back"_i18n, []() {});

    dialog->open();
}

NASBrowserActivity::~NASBrowserActivity() {
    brls::Logger::debug("NASBrowserActivity: delete");
    // Save current path before exit
    if (!currentPath.empty()) {
        saveCurrentPath();
    }
}
