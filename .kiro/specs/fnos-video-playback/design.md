# 设计文档

## 概述

本设计为 wiliwili 应用添加飞牛 NAS (FNOS) 视频播放功能。通过 WebDAV 协议访问飞牛 NAS 上的视频文件，用户可以浏览 NAS 文件夹结构并播放视频。

该功能将：
1. 提供 NAS 配置界面（服务器地址、用户名、密码）
2. 实现 WebDAV 客户端用于文件浏览
3. 复用现有的 MPV 播放器核心播放网络视频
4. 在主界面添加 NAS 视频入口按钮

### 为什么选择 WebDAV

飞牛 NAS 支持多种协议（SMB、WebDAV、FTP、NFS、DLNA），选择 WebDAV 的原因：
- **跨平台兼容性**: WebDAV 基于 HTTP，在所有平台上都有良好支持
- **MPV 原生支持**: MPV 可以直接播放 HTTP/HTTPS URL
- **简单实现**: 只需 HTTP 请求，无需额外的 SMB/NFS 库
- **防火墙友好**: 使用标准 HTTP 端口，穿透性好

## 架构

### 组件关系

```
MainActivity (主界面)
    └── NASVideoButton (新增按钮)
            └── Intent::openNASBrowser() (新增方法)
                    ├── NASConfigActivity (配置界面，首次使用)
                    └── NASBrowserActivity (文件浏览界面)
                            ├── WebDAVClient (WebDAV 客户端)
                            └── NASVideoPlayerActivity (视频播放)
                                    └── MPVCore (现有播放器核心)
```

### 数据流

```mermaid
sequenceDiagram
    participant User
    participant MainActivity
    participant NASBrowser
    participant WebDAV
    participant NAS
    participant VideoPlayer

    User->>MainActivity: 点击 NAS 按钮
    MainActivity->>NASBrowser: 打开浏览器
    NASBrowser->>WebDAV: PROPFIND 请求
    WebDAV->>NAS: HTTP 请求
    NAS-->>WebDAV: XML 响应
    WebDAV-->>NASBrowser: 文件列表
    NASBrowser-->>User: 显示文件列表
    User->>NASBrowser: 选择视频
    NASBrowser->>VideoPlayer: 传递视频 URL
    VideoPlayer->>NAS: 播放视频流
```

## 组件和接口

### 1. NAS 配置数据结构

```cpp
// wiliwili/include/api/nas/nas_config.hpp
struct NASConfig {
    std::string serverUrl;      // WebDAV 服务器地址，如 http://192.168.1.100:5005
    std::string username;       // 用户名
    std::string password;       // 密码
    std::string lastPath;       // 上次浏览路径
    bool enabled = false;       // 是否已配置
    
    // 构建完整的 WebDAV URL
    std::string buildUrl(const std::string& path) const;
    
    // 构建带认证的 URL（用于 MPV 播放）
    std::string buildAuthUrl(const std::string& path) const;
};
```

### 2. WebDAV 客户端

```cpp
// wiliwili/include/api/nas/webdav_client.hpp
class WebDAVItem {
public:
    std::string name;           // 文件/文件夹名
    std::string path;           // 完整路径
    bool isDirectory;           // 是否为目录
    int64_t size;               // 文件大小（字节）
    std::string contentType;    // MIME 类型
    std::string lastModified;   // 最后修改时间
};

class WebDAVClient {
public:
    WebDAVClient(const NASConfig& config);
    
    // 列出目录内容
    void listDirectory(
        const std::string& path,
        const std::function<void(std::vector<WebDAVItem>)>& callback,
        const std::function<void(const std::string&, int)>& error
    );
    
    // 检查连接是否有效
    void checkConnection(
        const std::function<void()>& success,
        const std::function<void(const std::string&, int)>& error
    );
    
private:
    NASConfig config;
    
    // 解析 PROPFIND XML 响应
    std::vector<WebDAVItem> parsePropfindResponse(const std::string& xml);
    
    // URL 编码
    static std::string urlEncode(const std::string& path);
    
    // 过滤视频文件
    static bool isVideoFile(const std::string& filename);
};
```

### 3. NAS 配置 Activity

```cpp
// wiliwili/include/activity/nas_config_activity.hpp
class NASConfigActivity : public brls::Activity {
public:
    CONTENT_FROM_XML_RES("activity/nas_config_activity.xml");
    
    void onContentAvailable() override;
    
private:
    BRLS_BIND(brls::InputCell, serverInput, "nas/server/input");
    BRLS_BIND(brls::InputCell, usernameInput, "nas/username/input");
    BRLS_BIND(brls::InputCell, passwordInput, "nas/password/input");
    BRLS_BIND(brls::Button, testButton, "nas/test/button");
    BRLS_BIND(brls::Button, saveButton, "nas/save/button");
    
    void testConnection();
    void saveConfig();
};
```

### 4. NAS 文件浏览 Activity

```cpp
// wiliwili/include/activity/nas_browser_activity.hpp
class NASBrowserActivity : public brls::Activity {
public:
    CONTENT_FROM_XML_RES("activity/nas_browser_activity.xml");
    
    NASBrowserActivity(const std::string& initialPath = "/");
    
    void onContentAvailable() override;
    
private:
    BRLS_BIND(RecyclingGrid, fileList, "nas/file/list");
    BRLS_BIND(brls::Label, pathLabel, "nas/path/label");
    
    std::string currentPath;
    std::unique_ptr<WebDAVClient> webdavClient;
    
    void loadDirectory(const std::string& path);
    void onItemSelected(const WebDAVItem& item);
    void navigateUp();
};
```

### 5. Intent 类扩展

```cpp
// 在 activity_helper.hpp 中添加
class Intent {
public:
    // ... 现有方法 ...
    
    // 打开 NAS 文件浏览器
    static void openNASBrowser(const std::string& path = "/");
    
    // 打开 NAS 配置界面
    static void openNASConfig();
    
    // 播放 NAS 视频
    static void playNASVideo(const std::string& url, const std::string& title);
};
```

## 数据模型

### NAS 配置存储

配置将存储在 ProgramConfig 中，添加新的 SettingItem：

```cpp
enum class SettingItem {
    // ... 现有项 ...
    NAS_SERVER_URL,
    NAS_USERNAME,
    NAS_PASSWORD,
    NAS_LAST_PATH,
    NAS_ENABLED,
};
```

### 支持的视频格式

```cpp
const std::vector<std::string> SUPPORTED_VIDEO_EXTENSIONS = {
    ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm",
    ".m4v", ".ts", ".m2ts", ".rmvb", ".rm"
};
```

## 正确性属性

*属性是一个特征或行为，应该在系统的所有有效执行中保持为真——本质上是关于系统应该做什么的形式化陈述。属性作为人类可读规范和机器可验证正确性保证之间的桥梁。*

### Property 1: NAS 地址格式验证

*对于任何*输入的服务器地址字符串，验证函数应正确识别有效的 IP 地址（如 192.168.1.100）、域名（如 nas.local）和带端口的地址（如 192.168.1.100:5005），并拒绝无效格式。
**Validates: Requirements 1.2**

### Property 2: 配置持久化往返

*对于任何*有效的 NAS 配置（服务器地址、用户名、密码），保存后重新加载应该得到完全相同的配置值。
**Validates: Requirements 1.4, 1.5**

### Property 3: WebDAV PROPFIND 响应解析

*对于任何*有效的 WebDAV PROPFIND XML 响应，解析器应正确提取所有文件和文件夹的名称、路径、大小和类型信息。
**Validates: Requirements 2.2, 4.4**

### Property 4: 视频文件过滤

*对于任何*文件列表，过滤后的结果应只包含支持的视频格式文件（mp4、mkv、avi、mov、wmv、flv、webm 等），且不遗漏任何符合条件的文件。
**Validates: Requirements 2.4**

### Property 5: 父目录导航

*对于任何*有效的文件路径，计算其父目录应返回正确的上级路径。根目录的父目录应为根目录本身。
**Validates: Requirements 2.5**

### Property 6: WebDAV URL 构建

*对于任何*服务器配置和文件路径组合，构建的 URL 应该是有效的 HTTP/HTTPS URL，包含正确编码的路径和认证信息。
**Validates: Requirements 3.1, 4.1, 4.2, 4.3**

### Property 7: 路径记忆往返

*对于任何*浏览路径，退出后重新进入应能恢复到相同的路径位置（如果路径仍然存在）。
**Validates: Requirements 6.1, 6.2**

## 错误处理

### 网络错误

| 错误类型 | 处理方式 |
|---------|---------|
| 连接超时 | 显示"无法连接到 NAS 服务器"，提供重试按钮 |
| 认证失败 (401) | 显示"用户名或密码错误"，引导到配置界面 |
| 路径不存在 (404) | 显示"文件夹不存在"，返回上级目录 |
| 服务器错误 (5xx) | 显示"服务器错误"，提供重试按钮 |

### 播放错误

| 错误类型 | 处理方式 |
|---------|---------|
| 视频格式不支持 | 显示"不支持的视频格式" |
| 缓冲超时 | 显示"网络不稳定"，允许重试 |
| 播放中断 | 显示"播放中断"，允许重试或返回 |

## 测试策略

### 单元测试

1. **地址验证测试**
   - 测试有效 IP 地址格式
   - 测试有效域名格式
   - 测试带端口的地址
   - 测试无效格式拒绝

2. **URL 编码测试**
   - 测试中文路径编码
   - 测试特殊字符编码
   - 测试空格编码

3. **XML 解析测试**
   - 测试标准 PROPFIND 响应解析
   - 测试空目录响应
   - 测试包含特殊字符的文件名

4. **文件过滤测试**
   - 测试各种视频格式识别
   - 测试大小写不敏感
   - 测试非视频文件过滤

### 属性测试

使用 C++ 的 RapidCheck 库进行属性测试：

1. **配置往返属性测试**
   - 生成随机配置，保存后加载验证一致性

2. **URL 构建属性测试**
   - 生成随机路径，验证 URL 格式正确性

3. **路径导航属性测试**
   - 生成随机路径，验证父目录计算正确性

### 测试框架

- 使用 Google Test 进行单元测试
- 使用 RapidCheck 进行属性测试
- 每个属性测试运行至少 100 次迭代
- 测试标注格式: `// Feature: fnos-video-playback, Property X: [属性描述]`

## 实现注意事项

### WebDAV 请求格式

PROPFIND 请求示例：
```http
PROPFIND /webdav/videos/ HTTP/1.1
Host: 192.168.1.100:5005
Authorization: Basic dXNlcm5hbWU6cGFzc3dvcmQ=
Depth: 1
Content-Type: application/xml

<?xml version="1.0" encoding="utf-8"?>
<propfind xmlns="DAV:">
  <prop>
    <displayname/>
    <getcontentlength/>
    <getcontenttype/>
    <getlastmodified/>
    <resourcetype/>
  </prop>
</propfind>
```

### 飞牛 NAS WebDAV 路径

飞牛 NAS 的 WebDAV 服务通常在 `/webdav/` 路径下，用户需要在飞牛 NAS 管理界面开启 WebDAV 服务并设置共享文件夹。

### 视频播放 URL 格式

MPV 播放 WebDAV 视频的 URL 格式：
```
http://username:password@192.168.1.100:5005/webdav/videos/movie.mp4
```

或使用 HTTP Header 认证（更安全）：
```cpp
// 设置 MPV 的 http-header-fields 选项
mpv_set_option_string(mpv, "http-header-fields", 
    "Authorization: Basic dXNlcm5hbWU6cGFzc3dvcmQ=");
```

### UI 布局文件

需要创建以下 XML 布局文件：
- `resources/xml/activity/nas_config_activity.xml` - 配置界面
- `resources/xml/activity/nas_browser_activity.xml` - 文件浏览界面
- `resources/xml/views/nas_file_item.xml` - 文件列表项

### 图标资源

需要准备以下 SVG 图标：
- `ico-nas.svg` - NAS 按钮普通状态
- `ico-nas-activate.svg` - NAS 按钮激活状态
- `ico-folder.svg` - 文件夹图标
- `ico-video-file.svg` - 视频文件图标
