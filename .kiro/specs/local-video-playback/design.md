# 设计文档

## 概述

本设计为 wiliwili 应用添加本地视频播放功能。通过在主界面添加一个新的图标按钮，用户可以点击该按钮播放存储在应用资源目录中的本地视频文件（network.mp4）。

该功能将复用现有的 MPV 播放器核心和播放器活动框架，只需要：
1. 扩展 Intent 类以支持本地文件播放
2. 创建一个简化的本地视频播放器活动
3. 在主界面添加触发按钮

## 架构

### 组件关系

```
MainActivity (主界面)
    └── LocalVideoButton (新增按钮)
            └── Intent::openLocalVideo() (新增方法)
                    └── LocalVideoPlayerActivity (新增活动)
                            └── MPVCore (现有播放器核心)
```

### 设计决策

1. **复用现有播放器**: 使用现有的 MPVCore 和 VideoView 组件，避免重复实现
2. **简化的播放器活动**: 创建一个轻量级的播放器活动，不需要评论、推荐等在线功能
3. **资源路径处理**: 使用 borealis 的资源管理系统获取本地文件的完整路径

## 组件和接口

### 1. Intent 类扩展

在 `activity_helper.hpp` 中添加新方法：

```cpp
class Intent {
public:
    // ... 现有方法 ...
    
    // 播放本地视频文件
    static void openLocalVideo(const std::string& filepath);
};
```

### 2. LocalVideoPlayerActivity

创建一个简化的播放器活动类：

```cpp
class LocalVideoPlayerActivity : public brls::Activity {
public:
    LocalVideoPlayerActivity(const std::string& filepath);
    ~LocalVideoPlayerActivity() override;
    
    void onContentAvailable() override;
    
private:
    std::string videoPath;
    BRLS_BIND(VideoView, video, "local_video/player");
};
```

**生命周期管理：**

析构函数必须在销毁前正确清理资源，遵循 `BasePlayerActivity` 的模式：

```cpp
LocalVideoPlayerActivity::~LocalVideoPlayerActivity() {
    // 1. 停止视频播放（防止 MPV 在 VideoView 销毁时仍在运行）
    if (this->video) {
        this->video->stop();
    }
    // 2. VideoView 会在父类析构时自动销毁
}
```

这个顺序至关重要，因为：
- MPV 播放器核心是单例，在多个 VideoView 之间共享
- 如果不先停止播放，MPV 可能在 VideoView 销毁后仍尝试访问已释放的资源
- VideoView 的析构函数会取消事件订阅，但这必须在 MPV 停止后进行

### 3. MainActivity 按钮扩展

在主界面 XML 布局中添加新按钮，并在 MainActivity 中注册点击事件。

## 数据模型

### 本地视频信息

```cpp
struct LocalVideoInfo {
    std::string filepath;      // 本地文件路径
    std::string displayName;   // 显示名称（可选）
};
```

对于当前需求，只需要一个固定的文件路径字符串即可。

## 正确性属性

*属性是一个特征或行为，应该在系统的所有有效执行中保持为真——本质上是关于系统应该做什么的形式化陈述。属性作为人类可读规范和机器可验证正确性保证之间的桥梁。*

### 属性 1: 点击按钮触发播放器

*对于任何*主界面状态，当用户点击本地视频按钮时，系统应该将一个播放器活动推送到应用栈
**验证: 需求 1.2**

### 属性 2: 正确的文件路径传递

*对于任何*本地视频播放请求，传递给 MPV 的文件路径应该是 resources/pictures/network.mp4 的完整路径
**验证: 需求 1.3**

### 属性 3: Intent 方法存在性

*对于*Intent 类，应该存在一个名为 openLocalVideo 的静态方法，接受字符串参数
**验证: 需求 2.1**

### 属性 4: 播放器活动创建

*对于任何*对 openLocalVideo 的调用，应该创建一个新的播放器活动实例
**验证: 需求 2.2**

### 属性 5: MPV 初始化

*对于任何*本地视频播放器活动，MPV 核心应该接收到正确的本地文件路径
**验证: 需求 2.3**

### 属性 6: 焦点状态图标切换

*对于任何*按钮焦点状态变化，图标应该在激活和普通状态之间正确切换
**验证: 需求 3.2, 3.3**

### 属性 7: 播放器退出时停止视频

*对于任何*播放器活动销毁事件，在 VideoView 被销毁前应该调用 stop() 方法停止 MPV 播放
**验证: 需求 4.2**

### 属性 8: 资源清理顺序

*对于任何*播放器活动销毁过程，资源清理应该按照以下顺序执行：停止视频播放 → 取消事件订阅 → 销毁 VideoView
**验证: 需求 4.3**

### 属性 9: 退出后应用稳定性

*对于任何*播放器退出操作，应用应该返回主界面且不发生崩溃
**验证: 需求 4.1, 4.5**

## 错误处理

### 文件不存在

如果指定的本地视频文件不存在：
- MPV 会自动处理并显示错误信息
- 播放器活动应该允许用户返回主界面

### MPV 初始化失败

如果 MPV 核心初始化失败：
- 记录错误日志
- 显示用户友好的错误提示
- 允许用户返回主界面

### 内存不足

如果系统内存不足无法加载视频：
- MPV 会尝试降低缓存大小
- 如果仍然失败，显示错误信息

## 测试策略

### 单元测试

1. **Intent 方法测试**
   - 测试 openLocalVideo 方法是否正确创建活动
   - 测试文件路径参数是否正确传递

2. **按钮点击测试**
   - 测试按钮点击事件是否正确触发 Intent 方法
   - 测试焦点事件是否正确更新图标状态

3. **播放器活动测试**
   - 测试活动是否正确初始化 VideoView
   - 测试文件路径是否正确传递给 MPV

### 集成测试

1. **端到端播放测试**
   - 从主界面点击按钮到视频开始播放的完整流程
   - 验证视频控制功能（播放、暂停、进度条）正常工作

2. **UI 集成测试**
   - 验证按钮在主界面的正确位置和样式
   - 验证焦点导航正常工作

### 测试框架

- 使用 C++ 的 Google Test 框架进行单元测试
- 使用 borealis 的测试工具进行 UI 测试
- 手动测试用于验证视频播放质量和用户体验

### 测试配置

- 单元测试应该运行至少 100 次迭代以确保稳定性
- 每个测试应该明确标注它验证的正确性属性
- 测试标注格式: `// Feature: local-video-playback, Property X: [属性描述]`

## 实现注意事项

### 资源路径获取

使用 borealis 的资源系统获取完整路径：

```cpp
std::string fullPath = brls::Application::getResourcesPath() + "pictures/network.mp4";
```

### XML 布局

本地视频播放器可以使用简化的 XML 布局，只包含 VideoView 组件，不需要评论、推荐等复杂组件。

### 图标资源

需要准备两个 SVG 图标：
- `ico-local-video.svg` - 普通状态
- `ico-local-video-activate.svg` - 激活状态

或者复用现有的视频相关图标。

### 按钮位置

建议将按钮放置在主界面的设置按钮和收件箱按钮附近，保持 UI 一致性。
