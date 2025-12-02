//
// Created by kiro on 2024/12/2.
//

#pragma once

#include <borealis/core/activity.hpp>
#include <borealis/core/bind.hpp>

namespace brls {
class RadioCell;
class InputCell;
class Label;
}  // namespace brls

/**
 * NAS 配置界面
 * 用于配置飞牛 NAS (FNOS) 的 WebDAV 连接信息
 */
class NASConfigActivity : public brls::Activity {
public:
    CONTENT_FROM_XML_RES("activity/nas_config_activity.xml");

    NASConfigActivity();

    void onContentAvailable() override;

    ~NASConfigActivity() override;

private:
    BRLS_BIND(brls::InputCell, serverInput, "nas/server/input");
    BRLS_BIND(brls::InputCell, usernameInput, "nas/username/input");
    BRLS_BIND(brls::InputCell, passwordInput, "nas/password/input");
    BRLS_BIND(brls::RadioCell, testButton, "nas/test/button");
    BRLS_BIND(brls::RadioCell, saveButton, "nas/save/button");
    BRLS_BIND(brls::Label, statusLabel, "nas/status/label");

    /**
     * 测试 NAS 连接
     */
    void testConnection();

    /**
     * 保存 NAS 配置
     */
    void saveConfig();

    /**
     * 显示状态消息
     * @param message 消息内容
     * @param isError 是否为错误消息
     */
    void showStatus(const std::string& message, bool isError = false);

    // 当前输入的配置值
    std::string currentServerUrl;
    std::string currentUsername;
    std::string currentPassword;
};
