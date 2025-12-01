//
// Created for local video playback feature
//

#pragma once

#include <borealis/core/activity.hpp>
#include <borealis/core/bind.hpp>

class VideoView;

class LocalVideoPlayerActivity : public brls::Activity {
public:
    // Constructor accepts the local video file path
    LocalVideoPlayerActivity(const std::string& filepath);
    
    ~LocalVideoPlayerActivity() override;
    
    // Called when the activity content is available
    void onContentAvailable() override;
    
    // XML layout resource
    CONTENT_FROM_XML_RES("activity/local_video_player_activity.xml");

private:
    std::string videoPath;
    BRLS_BIND(VideoView, video, "local_video/player");
};
