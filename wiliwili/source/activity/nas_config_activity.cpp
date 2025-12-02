//
// Created by kiro on 2024/12/2.
//

#include <borealis/core/i18n.hpp>
#include <borealis/core/application.hpp>
#include <borealis/core/thread.hpp>
#include <borealis/views/cells/cell_input.hpp>
#include <borealis/views/cells/cell_radio.hpp>
#include <borealis/views/label.hpp>

#include "activity/nas_config_activity.hpp"
#include "api/nas/nas_config.hpp"
#include "api/nas/webdav_client.hpp"
#include "utils/config_helper.hpp"

using namespace brls::literals;

NASConfigActivity::NASConfigActivity() {
    brls::Logger::debug("NASConfigActivity: create");
}

void NASConfigActivity::onContentAvailable() {
    brls::Logger::debug("NASConfigActivity: onContentAvailable");

    auto& conf = ProgramConfig::instance();
    NASConfig nasConfig = conf.getNASConfig();

    // 初始化当前值
    currentServerUrl = nasConfig.serverUrl;
    currentUsername = nasConfig.username;
    currentPassword = nasConfig.password;

    // 初始化服务器地址输入框
    serverInput->init(
        "wiliwili/nas/config/server"_i18n,
        currentServerUrl,
        [this](const std::string& data) {
            currentServerUrl = data;
        },
        "wiliwili/nas/config/server_hint"_i18n,
        "http://192.168.1.100:5005",
        256
    );

    // 初始化用户名输入框
    usernameInput->init(
        "wiliwili/nas/config/username"_i18n,
        currentUsername,
        [this](const std::string& data) {
            currentUsername = data;
        },
        "wiliwili/nas/config/username_hint"_i18n,
        "",
        64
    );

    // 初始化密码输入框
    passwordInput->init(
        "wiliwili/nas/config/password"_i18n,
        currentPassword.empty() ? "" : "********",
        [this](const std::string& data) {
            // 只有当用户实际输入了新密码时才更新
            if (data != "********") {
                currentPassword = data;
            }
        },
        "wiliwili/nas/config/password_hint"_i18n,
        "",
        64
    );

    // 测试连接按钮
    testButton->registerClickAction([this](...) -> bool {
        testConnection();
        return true;
    });

    // 保存配置按钮
    saveButton->registerClickAction([this](...) -> bool {
        saveConfig();
        return true;
    });

    // 清空状态标签
    statusLabel->setText("");
}

void NASConfigActivity::testConnection() {
    // 验证服务器地址
    if (currentServerUrl.empty()) {
        showStatus("wiliwili/nas/error/empty_server"_i18n, true);
        return;
    }

    if (!NASConfig::isValidServerAddress(currentServerUrl)) {
        showStatus("wiliwili/nas/error/invalid_server"_i18n, true);
        return;
    }

    showStatus("wiliwili/nas/status/testing"_i18n, false);

    // 创建临时配置用于测试
    NASConfig testConfig;
    testConfig.serverUrl = currentServerUrl;
    testConfig.username = currentUsername;
    testConfig.password = currentPassword;

    // 创建 WebDAV 客户端并测试连接
    auto client = std::make_shared<WebDAVClient>(testConfig);
    client->checkConnection(
        [this]() {
            brls::sync([this]() {
                showStatus("wiliwili/nas/status/success"_i18n, false);
            });
        },
        [this](const std::string& error, int code) {
            brls::sync([this, error, code]() {
                std::string errorMsg;
                switch (code) {
                    case 401:
                        errorMsg = "wiliwili/nas/error/auth_failed"_i18n;
                        break;
                    case 404:
                        errorMsg = "wiliwili/nas/error/path_not_found"_i18n;
                        break;
                    case 500:
                    case 502:
                    case 503:
                    case 504:
                        errorMsg = "wiliwili/nas/error/server_error"_i18n + " (" + std::to_string(code) + ")";
                        break;
                    case -1:
                        // Connection error (timeout, network unreachable, etc.)
                        if (error.find("timeout") != std::string::npos || 
                            error.find("Timeout") != std::string::npos ||
                            error.find("timed out") != std::string::npos) {
                            errorMsg = "wiliwili/nas/error/timeout"_i18n;
                        } else if (error.find("resolve") != std::string::npos ||
                                   error.find("host") != std::string::npos) {
                            errorMsg = "wiliwili/nas/error/network_error"_i18n + ": " + error;
                        } else {
                            errorMsg = "wiliwili/nas/error/connection_failed"_i18n + ": " + error;
                        }
                        break;
                    default:
                        errorMsg = "wiliwili/nas/error/connection_failed"_i18n + ": " + error;
                        break;
                }
                showStatus(errorMsg, true);
            });
        }
    );
}

void NASConfigActivity::saveConfig() {
    // 验证服务器地址
    if (currentServerUrl.empty()) {
        showStatus("wiliwili/nas/error/empty_server"_i18n, true);
        return;
    }

    if (!NASConfig::isValidServerAddress(currentServerUrl)) {
        showStatus("wiliwili/nas/error/invalid_server"_i18n, true);
        return;
    }

    // 保存配置
    auto& conf = ProgramConfig::instance();
    NASConfig nasConfig;
    nasConfig.serverUrl = currentServerUrl;
    nasConfig.username = currentUsername;
    nasConfig.password = currentPassword;
    nasConfig.enabled = true;
    nasConfig.lastPath = "/";

    conf.setNASConfig(nasConfig);

    showStatus("wiliwili/nas/status/saved"_i18n, false);
    brls::Application::notify("wiliwili/nas/status/saved"_i18n);
}

void NASConfigActivity::showStatus(const std::string& message, bool isError) {
    if (statusLabel) {
        statusLabel->setText(message);
        if (isError) {
            statusLabel->setTextColor(nvgRGB(255, 100, 100));
        } else {
            statusLabel->setTextColor(brls::Application::getTheme()["brls/text"]);
        }
    }
}

NASConfigActivity::~NASConfigActivity() {
    brls::Logger::debug("NASConfigActivity: delete");
}
