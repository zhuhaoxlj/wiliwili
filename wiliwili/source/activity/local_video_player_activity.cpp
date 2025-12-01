//
// Created for local video playback feature
//

#include "activity/local_video_player_activity.hpp"
#include "view/video_view.hpp"
#include <borealis/core/logger.hpp>
#include <borealis/core/application.hpp>

LocalVideoPlayerActivity::LocalVideoPlayerActivity(const std::string& filepath) 
    : videoPath(filepath) {
    brls::Logger::debug("LocalVideoPlayerActivity created with path: {}", filepath);
}

LocalVideoPlayerActivity::~LocalVideoPlayerActivity() {
    brls::Logger::debug("LocalVideoPlayerActivity destroyed");
    // 注意：不要在这里调用 video->stop()
    // MPV 的 stop 命令是异步的，可能在 VideoView 销毁后才触发事件
    // 这会导致访问已释放的内存而崩溃
    // VideoView 的析构函数会自动取消 MPV 事件订阅，这足以安全地清理资源
}

void LocalVideoPlayerActivity::onContentAvailable() {
    brls::Logger::debug("LocalVideoPlayerActivity::onContentAvailable");
    
    brls::Logger::info("Loading local video from: {}", videoPath);
    
    // Set the video URL to the local file path
    // The VideoView will handle the MPV core initialization and playback
    this->video->setUrl(videoPath);
    
    // Set a simple title for the video
    this->video->setTitle("Local Video");
    
    // Hide unnecessary UI elements for local playback
    this->video->hideDanmakuButton();
    this->video->hideVideoQualityButton();
    this->video->hideHistorySetting();
    this->video->hideVideoRelatedSetting();
    
    // Register common video player actions (play/pause, seek, etc.)
    this->video->registerCommonActions(this);
    
    // 注意：不需要调用 registerMpvEvent()，因为 VideoView 构造函数已经注册了
    // 重复调用会导致 "VideoView already register MPV Event" 错误
    
    // 在 VideoView 上覆盖 B 键处理
    // 注意：LocalVideoPlayerActivity 的 VideoView 布局本身就是全屏的（100% 宽高）
    // 所以 isFullscreen() 会始终返回 true（它是基于视图尺寸判断的）
    // 因此不能使用 isFullscreen() 来判断是否需要退出，而是直接退出 Activity
    brls::Logger::info("LocalVideoPlayerActivity: Registering B button handler on VideoView");
    this->video->registerAction(
        "hints/back"_i18n, brls::ControllerButton::BUTTON_B,
        [this](brls::View* view) -> bool {
            brls::Logger::info("LocalVideoPlayerActivity: B button pressed on VideoView, isOSDLock={}", 
                this->video->isOSDLock());
            // 检查 OSD 锁定状态
            if (this->video->isOSDLock()) {
                brls::Logger::info("LocalVideoPlayerActivity: OSD is locked, toggling OSD");
                this->video->toggleOSD();
                return true;
            }
            // 直接退出 Activity（本地视频播放器没有"非全屏"模式）
            brls::Logger::info("LocalVideoPlayerActivity: Back button pressed, exiting activity");
            brls::Application::popActivity();
            return true;
        },
        true
    );
    
    // 同时在 Activity 的 contentView 上注册 B 键处理
    // 这样当焦点在 ButtonClose 或其他子视图上时，B 键也能正确退出
    brls::Logger::info("LocalVideoPlayerActivity: Registering B button handler on Activity contentView");
    this->registerAction(
        "hints/back"_i18n, brls::ControllerButton::BUTTON_B,
        [this](brls::View* view) -> bool {
            brls::Logger::info("LocalVideoPlayerActivity: B button pressed on Activity contentView, isOSDLock={}", 
                this->video->isOSDLock());
            // 检查 OSD 锁定状态
            if (this->video->isOSDLock()) {
                brls::Logger::info("LocalVideoPlayerActivity: OSD is locked, toggling OSD");
                this->video->toggleOSD();
                return true;
            }
            // 直接退出 Activity
            brls::Logger::info("LocalVideoPlayerActivity: Back button pressed, exiting activity");
            brls::Application::popActivity();
            return true;
        },
        true
    );
}
