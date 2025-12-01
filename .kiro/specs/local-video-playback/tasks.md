# 实现计划

- [x] 1. 扩展 Intent 类以支持本地视频播放
  - 在 `wiliwili/include/utils/activity_helper.hpp` 中添加 `openLocalVideo` 静态方法声明
  - 在 `wiliwili/source/utils/activity_helper.cpp` 中实现 `openLocalVideo` 方法
  - 方法应接受文件路径参数并创建本地视频播放器活动
  - _需求: 2.1, 2.2_

- [x] 2. 创建本地视频播放器活动
  - 创建 `wiliwili/include/activity/local_video_player_activity.hpp` 头文件
  - 创建 `wiliwili/source/activity/local_video_player_activity.cpp` 实现文件
  - 实现 LocalVideoPlayerActivity 类，继承自 brls::Activity
  - 在构造函数中接受文件路径参数
  - 实现 onContentAvailable 方法初始化 VideoView
  - 将文件路径传递给 MPV 核心进行播放
  - _需求: 2.3, 1.3_

- [x] 3. 创建播放器 XML 布局文件
  - 创建 `resources/xml/activity/local_video_player_activity.xml`
  - 定义简化的布局，只包含 VideoView 组件
  - 配置 VideoView 的基本属性（全屏、控制器等）
  - _需求: 1.4_

- [x] 4. 在主界面添加本地视频按钮
  - 修改主界面 XML 布局文件 `resources/xml/activity/main_activity.xml`
  - 添加新的按钮组件，使用合适的图标
  - 设置按钮的布局位置和样式，与现有按钮保持一致
  - _需求: 1.1, 3.1_

- [x] 5. 在 MainActivity 中注册按钮事件
  - 修改 `wiliwili/source/activity/main_activity.cpp`
  - 绑定新添加的按钮组件
  - 注册点击事件，调用 `Intent::openLocalVideo`
  - 传递正确的本地视频文件路径 (resources/pictures/network.mp4)
  - _需求: 1.2, 1.3_

- [x] 6. 实现按钮焦点状态切换
  - 在 MainActivity 中订阅按钮的焦点事件
  - 当按钮获得焦点时，切换到激活状态的图标
  - 当按钮失去焦点时，切换回普通状态的图标
  - 添加触摸手势识别器支持触摸操作
  - _需求: 3.2, 3.3, 3.4_

- [x] 7. 配置按钮导航
  - 设置按钮的自定义导航逻辑
  - 确保按钮可以通过手柄正确导航到其他 UI 元素
  - 与现有的 settingBtn 和 inboxBtn 保持一致的导航行为
  - _需求: 3.4_

- [ ]* 8. 编写单元测试
  - 测试 Intent::openLocalVideo 方法是否正确创建活动
  - 测试文件路径参数是否正确传递
  - 测试按钮点击事件是否正确触发
  - 测试焦点事件是否正确更新图标状态
  - _需求: 1.2, 2.1, 2.2, 3.2, 3.3_

- [ ]* 9. 编写集成测试
  - 测试从主界面点击按钮到视频播放的完整流程
  - 验证视频控制功能正常工作
  - 验证按钮在主界面的位置和样式
  - 验证焦点导航正常工作
  - _需求: 1.2, 1.3, 1.4, 3.1, 3.4_

- [x] 10. 修复播放器退出时的崩溃问题
  - 在 `LocalVideoPlayerActivity` 析构函数中添加视频停止调用
  - 参考 `BasePlayerActivity` 的实现模式
  - 确保在 VideoView 销毁前调用 `this->video->stop()`
  - 添加空指针检查以防止潜在的空引用
  - _需求: 4.1, 4.2, 4.3, 4.5_

- [ ] 11. 检查点 - 确保所有测试通过
  - 确保所有测试通过，如有问题请询问用户
  - 特别测试播放器退出时的稳定性
