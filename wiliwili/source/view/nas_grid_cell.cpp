//
// Created by kiro on 2024/12/3.
//

#include "view/nas_grid_cell.hpp"
#include "view/svg_image.hpp"
#include "api/nas/webdav_client.hpp"
#include "utils/image_helper.hpp"

#include <borealis/core/i18n.hpp>

using namespace brls::literals;

NASGridCell::NASGridCell() {
    this->inflateFromXMLRes("xml/views/nas_grid_cell.xml");
}

NASGridCell::~NASGridCell() {
    ImageHelper::clear(this->thumbnail);
}

void NASGridCell::setItem(const WebDAVItem& item) {
    // Set filename
    if (nameLabel) {
        nameLabel->setText(item.name);
    }

    // Set size label
    if (sizeLabel) {
        if (item.isDirectory) {
            sizeLabel->setText("wiliwili/nas/browser/folder"_i18n);
        } else {
            sizeLabel->setText(formatFileSize(item.size));
        }
    }

    // Show placeholder icon based on type
    showPlaceholder(item.isDirectory);
}

void NASGridCell::showPlaceholder(bool isFolder) {
    if (placeholderIcon) {
        placeholderIcon->setVisibility(brls::Visibility::VISIBLE);
        if (isFolder) {
            placeholderIcon->setImageFromSVGRes("svg/ico-folder.svg");
        } else {
            placeholderIcon->setImageFromSVGRes("svg/play-circle-video.svg");
        }
    }
    
    if (thumbnail) {
        thumbnail->setVisibility(brls::Visibility::GONE);
    }
}

void NASGridCell::setThumbnail(const std::string& url) {
    if (url.empty()) {
        return;
    }

    if (thumbnail) {
        thumbnail->setVisibility(brls::Visibility::VISIBLE);
        ImageHelper::with(this->thumbnail)->load(url);
    }
    
    if (placeholderIcon) {
        placeholderIcon->setVisibility(brls::Visibility::GONE);
    }
}

void NASGridCell::prepareForReuse() {
    // Reset to default state
    if (thumbnail) {
        thumbnail->setImageFromRes("pictures/video-card-bg.png");
        thumbnail->setVisibility(brls::Visibility::GONE);
    }
    if (placeholderIcon) {
        placeholderIcon->setVisibility(brls::Visibility::VISIBLE);
    }
}

void NASGridCell::cacheForReuse() {
    ImageHelper::clear(this->thumbnail);
}

NASGridCell* NASGridCell::create() {
    return new NASGridCell();
}

std::string NASGridCell::formatFileSize(int64_t size) {
    if (size < 1024) {
        return std::to_string(size) + " B";
    } else if (size < 1024 * 1024) {
        return std::to_string(size / 1024) + " KB";
    } else if (size < 1024LL * 1024 * 1024) {
        return std::to_string(size / (1024 * 1024)) + " MB";
    } else {
        return std::to_string(size / (1024LL * 1024 * 1024)) + " GB";
    }
}
