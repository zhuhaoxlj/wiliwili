# Design Document: NAS Browser Thumbnail View

## Overview

为 NAS 浏览器添加网格视图模式，允许用户以缩略图形式浏览视频文件。该功能复用现有的 `RecyclingGrid` 组件和视频卡片设计模式，在 header 区域添加视图切换按钮。

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    NASBrowserActivity                        │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────┐    │
│  │                    Header                            │    │
│  │  [Path Label]              [Status] [Toggle Button]  │    │
│  └─────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              Content Area (switchable)               │    │
│  │                                                      │    │
│  │   List View (RecyclerFrame)                         │    │
│  │        OR                                            │    │
│  │   Grid View (RecyclingGrid)                         │    │
│  │                                                      │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. View Mode Enum

```cpp
enum class NASViewMode {
    List,   // 列表视图（当前模式）
    Grid    // 网格视图（缩略图模式）
};
```

### 2. NASBrowserActivity 扩展

在现有的 `NASBrowserActivity` 中添加：

- `viewMode`: 当前视图模式
- `gridView`: RecyclingGrid 组件指针
- `toggleViewMode()`: 切换视图模式的方法
- `updateViewVisibility()`: 根据模式显示/隐藏对应视图

### 3. NASGridCell (新组件)

用于网格视图的视频/文件夹卡片：

```cpp
class NASGridCell : public RecyclingGridItem {
public:
    void setItem(const WebDAVItem& item);
    void setThumbnail(const std::string& url);
    void showPlaceholder(bool isFolder);
    
private:
    brls::Image* thumbnail;
    brls::Label* nameLabel;
    brls::Label* sizeLabel;
};
```

### 4. NASGridDataSource

网格视图的数据源：

```cpp
class NASGridDataSource : public RecyclingGridDataSource {
public:
    size_t getItemCount() override;
    RecyclingGridItem* cellForRow(RecyclingGrid* recycler, size_t index) override;
    void onItemSelected(RecyclingGrid* recycler, size_t index) override;
};
```

### 5. View Toggle Button

在 header 区域添加 SVG 图标按钮，点击切换视图模式：
- 列表图标：当前为网格模式时显示
- 网格图标：当前为列表模式时显示

## Data Models

### WebDAVItem (现有，无需修改)

```cpp
struct WebDAVItem {
    std::string name;
    std::string path;
    bool isDirectory;
    size_t size;
    // ... 其他字段
};
```

### NASConfig 扩展

在现有配置中添加视图模式偏好：

```cpp
struct NASConfig {
    // ... 现有字段
    NASViewMode viewMode = NASViewMode::List;  // 新增
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: View mode toggle is idempotent cycle
*For any* initial view mode, toggling twice should return to the original mode.
**Validates: Requirements 1.2**

### Property 2: View mode persistence round-trip
*For any* view mode value, saving to config then loading from config should return the same view mode.
**Validates: Requirements 1.5**

### Property 3: Video card displays filename
*For any* WebDAVItem representing a video file, the rendered NASGridCell should contain the item's filename.
**Validates: Requirements 2.3**

### Property 4: Video card displays file size
*For any* WebDAVItem representing a video file, the rendered NASGridCell should contain the item's formatted file size.
**Validates: Requirements 2.4**

### Property 5: Folder card displays folder name
*For any* WebDAVItem representing a folder, the rendered NASGridCell should contain the folder's name.
**Validates: Requirements 3.2**

### Property 6: Folder selection navigates
*For any* folder item in the grid view, selecting it should update the current path to that folder's path.
**Validates: Requirements 3.3**

## Error Handling

1. **缩略图加载失败**: 显示占位图，不影响其他项目
2. **视图切换时数据为空**: 保持空状态提示
3. **配置保存失败**: 静默失败，使用默认值

## Testing Strategy

### Unit Tests

- 测试 `NASGridCell::setItem()` 正确设置文件名和大小
- 测试 `toggleViewMode()` 正确切换状态
- 测试配置序列化/反序列化

### Property-Based Tests

使用项目现有的测试框架，为上述 Correctness Properties 编写属性测试：

1. **Property 1**: 生成随机初始模式，验证双次切换返回原值
2. **Property 2**: 生成随机视图模式，验证配置保存/加载的一致性
3. **Property 3-5**: 生成随机 WebDAVItem，验证渲染结果包含必要信息
4. **Property 6**: 生成随机文件夹项，验证选择后路径更新

### Integration Tests

- 测试完整的视图切换流程
- 测试网格视图滚动和项目选择
