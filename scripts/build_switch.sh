#!/bin/bash
set -e

BUILD_DIR=cmake-build-switch

# cd to wiliwili
cd "$(dirname $0)/.."
git config --global --add safe.directory `pwd`

BASE_URL="https://github.com/xfangfang/wiliwili/releases/download/v0.1.0/"

# 安装必需的系统包
echo "安装必需的系统包..."
dkp-pacman -S --noconfirm --needed switch-curl switch-libplacebo

# DEKO3D 版本需要特殊的 libmpv 包
PKGS=(
    "libuam-f8c9eef01ffe06334d530393d636d69e2b52744b-1-any.pkg.tar.zst"
    "switch-ffmpeg-7.1-1-any.pkg.tar.zst"
    "switch-libmpv_deko3d-0.36.0-2-any.pkg.tar.zst"
    "switch-nspmini-48d4fc2-1-any.pkg.tar.xz"
    "hacBrewPack-3.05-1-any.pkg.tar.zst"
)
for PKG in "${PKGS[@]}"; do
    [ -f "${PKG}" ] || curl -LO ${BASE_URL}${PKG}
    dkp-pacman -U --noconfirm ${PKG}
done

# 设置 PKG_CONFIG_LIBDIR 以便 cmake 能找到 switch 库的 .pc 文件
export PKG_CONFIG_LIBDIR=/opt/devkitpro/portlibs/switch/lib/pkgconfig
echo "PKG_CONFIG_LIBDIR 设置为: $PKG_CONFIG_LIBDIR"

# 注意：在 macOS 上禁用 BUILTIN_NSP，因为 hacbrewpack 是 Linux x86-64 二进制文件
# 使用 DEKO3D 图形驱动而不是 GLFW（GLFW 在 Switch 上不可用）
cmake -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=Release -DBUILTIN_NSP=OFF -DUSE_DEKO3D=ON -DPLATFORM_SWITCH=ON -DBRLS_UNITY_BUILD=ON -DCMAKE_UNITY_BUILD_BATCH_SIZE=16
make -C ${BUILD_DIR} wiliwili.nro -j$(nproc)