//
// Created by kiro on 2024/12/3.
//

#pragma once

#include "view/recycling_grid.hpp"

class WebDAVItem;
class SVGImage;

namespace brls {
class Image;
class Label;
class Box;
}  // namespace brls

/**
 * NAS 网格视图单元格
 * 用于在网格视图中显示视频文件或文件夹
 */
class NASGridCell : public RecyclingGridItem {
public:
    NASGridCell();
    ~NASGridCell() override;

    /**
     * 设置单元格内容
     * @param item WebDAV 文件/文件夹项
     */
    void setItem(const WebDAVItem& item);

    /**
     * 显示占位图
     * @param isFolder 是否为文件夹
     */
    void showPlaceholder(bool isFolder);

    /**
     * 设置缩略图
     * @param url 缩略图 URL
     */
    void setThumbnail(const std::string& url);

    void prepareForReuse() override;
    void cacheForReuse() override;

    static NASGridCell* create();

    /**
     * 格式化文件大小
     * @param size 文件大小（字节）
     * @return 格式化后的字符串
     */
    static std::string formatFileSize(int64_t size);

private:
    BRLS_BIND(brls::Image, thumbnail, "nas/grid/thumbnail");
    BRLS_BIND(brls::Label, nameLabel, "nas/grid/name");
    BRLS_BIND(brls::Label, sizeLabel, "nas/grid/size");
    BRLS_BIND(SVGImage, placeholderIcon, "nas/grid/placeholder");
    BRLS_BIND(brls::Box, thumbnailBox, "nas/grid/thumbnail_box");
};
