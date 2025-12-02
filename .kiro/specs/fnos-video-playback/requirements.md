# 需求文档

## 简介

为 wiliwili 应用添加飞牛 NAS (FNOS) 视频播放功能，允许用户通过网络协议（SMB/WebDAV）访问并播放存储在飞牛 NAS 上的视频文件。该功能将扩展现有的本地视频播放功能，支持远程 NAS 视频源。

## 术语表

- **飞牛 NAS (FNOS)**: 一款国产 NAS 操作系统，支持 SMB、WebDAV、FTP、NFS、DLNA 等多种文件共享协议
- **SMB**: Server Message Block 协议，用于局域网文件共享
- **WebDAV**: Web-based Distributed Authoring and Versioning，基于 HTTP 的文件访问协议
- **wiliwili**: 一个基于 borealis 框架的视频播放应用
- **MPVCore**: 视频播放核心组件，支持通过 URL 播放网络视频
- **NAS URL**: 访问 NAS 视频的网络地址，格式如 `smb://ip/share/path` 或 `http://ip:port/path`

## 需求

### 需求 1

**用户故事:** 作为用户，我想要配置飞牛 NAS 的连接信息，以便应用能够访问我的 NAS 视频文件

#### 验收标准

1. WHEN 用户打开 NAS 设置界面 THEN 系统应显示 NAS 服务器地址输入框
2. WHEN 用户输入 NAS 地址 THEN 系统应支持 IP 地址或域名格式
3. WHEN 用户配置认证信息 THEN 系统应提供用户名和密码输入框
4. WHEN 用户保存配置 THEN 系统应将配置持久化存储到本地
5. WHEN 用户重新打开应用 THEN 系统应自动加载已保存的 NAS 配置

### 需求 2

**用户故事:** 作为用户，我想要浏览飞牛 NAS 上的视频文件列表，以便选择要播放的视频

#### 验收标准

1. WHEN 用户点击 NAS 视频按钮 THEN 系统应连接到配置的 NAS 服务器
2. WHEN 连接成功 THEN 系统应显示 NAS 共享文件夹列表
3. WHEN 用户进入文件夹 THEN 系统应显示该文件夹下的子文件夹和视频文件
4. WHEN 显示视频文件 THEN 系统应过滤并只显示支持的视频格式（mp4、mkv、avi、mov、wmv、flv、webm）
5. WHEN 用户返回上级目录 THEN 系统应正确导航到父文件夹

### 需求 3

**用户故事:** 作为用户，我想要播放飞牛 NAS 上的视频文件，以便观看我存储在 NAS 上的视频内容

#### 验收标准

1. WHEN 用户选择一个视频文件 THEN 系统应构建正确的视频 URL
2. WHEN 系统播放 NAS 视频 THEN 系统应使用 MPV 播放器通过网络 URL 加载视频
3. WHEN 视频播放器打开 THEN 系统应提供标准的播放控制功能（播放、暂停、进度条、音量等）
4. WHEN 视频正在缓冲 THEN 系统应显示加载指示器
5. WHEN 网络中断 THEN 系统应显示错误提示并允许用户重试或返回

### 需求 4

**用户故事:** 作为用户，我想要通过 WebDAV 协议访问飞牛 NAS，以便在不支持 SMB 的环境下也能播放视频

#### 验收标准

1. WHEN 用户选择 WebDAV 协议 THEN 系统应使用 HTTP/HTTPS URL 格式访问 NAS
2. WHEN 构建 WebDAV URL THEN 系统应正确编码路径中的特殊字符
3. WHEN WebDAV 需要认证 THEN 系统应在 URL 中包含认证信息或使用 HTTP Basic Auth
4. WHEN 列出 WebDAV 目录 THEN 系统应解析 WebDAV PROPFIND 响应获取文件列表

### 需求 5

**用户故事:** 作为用户，我想要在主界面快速访问 NAS 视频功能，以便方便地浏览和播放 NAS 视频

#### 验收标准

1. WHEN 用户在主界面 THEN 系统应显示一个 NAS 视频入口按钮
2. WHEN NAS 未配置时点击按钮 THEN 系统应引导用户进入 NAS 配置界面
3. WHEN NAS 已配置时点击按钮 THEN 系统应直接进入 NAS 文件浏览界面
4. WHEN 按钮获得焦点 THEN 系统应显示激活状态的图标
5. WHEN 按钮失去焦点 THEN 系统应显示普通状态的图标

### 需求 6

**用户故事:** 作为用户，我想要应用记住我上次浏览的位置，以便快速继续浏览

#### 验收标准

1. WHEN 用户退出 NAS 浏览界面 THEN 系统应记录当前浏览路径
2. WHEN 用户再次进入 NAS 浏览界面 THEN 系统应提供选项恢复到上次浏览位置
3. WHEN 上次路径不再存在 THEN 系统应回退到根目录并提示用户

