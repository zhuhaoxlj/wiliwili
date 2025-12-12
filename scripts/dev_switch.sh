#!/bin/bash
#
# wiliwili Switch 开发脚本
# 支持首次环境部署、增量编译、部署到 Switch
#
# 用法:
#   ./scripts/dev_switch.sh setup     # 首次环境部署（安装依赖库）
#   ./scripts/dev_switch.sh build     # 增量编译（日常开发用）
#   ./scripts/dev_switch.sh rebuild   # 完全重新编译
#   ./scripts/dev_switch.sh deploy    # 部署到 Switch（需要指定 IP）
#   ./scripts/dev_switch.sh all       # setup + build + deploy
#
# 环境变量:
#   SWITCH_IP=192.168.x.x    # Switch 的 IP 地址
#   JOBS=4                   # 并行编译数（默认 4）
#

set -e

# ==================== 用户配置 ====================
# 修改这里的配置，不需要每次输入环境变量

# Switch 的 IP 地址（在 Switch 上打开 Homebrew Menu 可以看到）
SWITCH_IP_DEFAULT="192.168.125.4"

# 并行编译数（建议设置为 CPU 核心数）
JOBS_DEFAULT=16

# ==================== 以下不需要修改 ====================

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置（环境变量优先，否则使用默认值）
BUILD_DIR="cmake-build-switch"
JOBS=${JOBS:-$JOBS_DEFAULT}
SWITCH_IP=${SWITCH_IP:-$SWITCH_IP_DEFAULT}

# 切换到项目根目录
cd "$(dirname "$0")/.."
PROJECT_DIR=$(pwd)

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查 devkitPro 环境
check_devkitpro() {
    if [ ! -d "/opt/devkitpro" ]; then
        log_error "devkitPro 未安装！请先安装 devkitPro"
        log_info "macOS 安装方法: brew install --cask devkitpro-pacman"
        exit 1
    fi
    
    if ! command -v dkp-pacman &> /dev/null; then
        log_error "dkp-pacman 命令不存在"
        exit 1
    fi
    
    export DEVKITPRO=/opt/devkitpro
    export DEVKITARM=/opt/devkitpro/devkitARM
    export DEVKITPPC=/opt/devkitpro/devkitPPC
    export PATH=$DEVKITPRO/tools/bin:$PATH
}

# 安装系统依赖
install_system_deps() {
    log_info "安装系统依赖包..."
    # 注意：switch-fribidi, switch-libass 等字幕相关库通过预编译包安装
    sudo dkp-pacman -S --noconfirm --needed \
        switch-dev \
        switch-curl \
        switch-libplacebo \
        switch-sdl2 \
        switch-mesa \
        switch-mbedtls \
        switch-zlib \
        switch-bzip2 \
        switch-libpng \
        switch-freetype \
        switch-harfbuzz \
        devkitA64
}

# 安装预编译的依赖包（带字幕支持的 DEKO3D 版本）
install_prebuilt_deps() {
    log_info "安装预编译依赖包..."
    
    BASE_URL="https://github.com/xfangfang/wiliwili/releases/download/v0.1.0/"
    
    PKGS=(
        "libuam-f8c9eef01ffe06334d530393d636d69e2b52744b-1-any.pkg.tar.zst"
        "switch-ffmpeg-7.1-1-any.pkg.tar.zst"
        "switch-libmpv_deko3d-0.36.0-2-any.pkg.tar.zst"
        "switch-nspmini-48d4fc2-1-any.pkg.tar.xz"
        "hacBrewPack-3.05-1-any.pkg.tar.zst"
    )
    
    for PKG in "${PKGS[@]}"; do
        if [ ! -f "${PKG}" ]; then
            log_info "下载 ${PKG}..."
            curl -LO "${BASE_URL}${PKG}"
        fi
        sudo dkp-pacman -U --noconfirm "${PKG}"
    done
}

# 从源码编译字幕支持库（可选，如果预编译包不满足需求）
build_libass_from_source() {
    log_info "从源码编译 libass 支持..."
    # 这里可以调用 build_switch_with_libass.sh 的相关逻辑
    # 但通常预编译包已经足够
    log_warn "暂未实现，请使用预编译包或手动运行 build_switch_with_libass.sh"
}

# 配置 CMake
configure_cmake() {
    log_info "配置 CMake..."
    
    export PKG_CONFIG_LIBDIR=/opt/devkitpro/portlibs/switch/lib/pkgconfig
    
    cmake -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DPLATFORM_SWITCH=ON \
        -DUSE_DEKO3D=ON \
        -DBUILTIN_NSP=OFF \
        -DBRLS_UNITY_BUILD=ON \
        -DCMAKE_UNITY_BUILD_BATCH_SIZE=16
    
    log_success "CMake 配置完成"
}

# 编译
do_build() {
    log_info "开始编译 (并行数: ${JOBS})..."
    
    export PKG_CONFIG_LIBDIR=/opt/devkitpro/portlibs/switch/lib/pkgconfig
    
    # 检查是否需要配置
    if [ ! -f "${BUILD_DIR}/Makefile" ]; then
        configure_cmake
    fi
    
    make -C "${BUILD_DIR}" wiliwili.nro -j${JOBS}
    
    log_success "编译完成: ${BUILD_DIR}/wiliwili.nro"
}

# 完全重新编译
do_rebuild() {
    log_info "清理并重新编译..."
    
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
    fi
    
    configure_cmake
    do_build
}

# 部署到 Switch
do_deploy() {
    if [ -z "${SWITCH_IP}" ]; then
        log_error "请设置 SWITCH_IP 环境变量"
        log_info "用法: SWITCH_IP=192.168.x.x ./scripts/dev_switch.sh deploy"
        exit 1
    fi
    
    if [ ! -f "${BUILD_DIR}/wiliwili.nro" ]; then
        log_error "未找到编译产物，请先编译"
        exit 1
    fi
    
    log_info "部署到 Switch (${SWITCH_IP})..."
    nxlink -s -a "${SWITCH_IP}" "${BUILD_DIR}/wiliwili.nro"
}

# 首次环境部署
do_setup() {
    log_info "========== 首次环境部署 =========="
    
    check_devkitpro
    install_system_deps
    install_prebuilt_deps
    configure_cmake
    
    log_success "环境部署完成！"
    log_info "现在可以运行: ./scripts/dev_switch.sh build"
}

# 显示帮助
show_help() {
    echo "wiliwili Switch 开发脚本"
    echo ""
    echo "用法: ./scripts/dev_switch.sh <命令>"
    echo ""
    echo "命令:"
    echo "  setup     首次环境部署（安装依赖库）"
    echo "  build     增量编译（日常开发用）"
    echo "  rebuild   完全重新编译"
    echo "  deploy    部署到 Switch"
    echo "  all       setup + build + deploy"
    echo "  help      显示此帮助"
    echo ""
    echo "环境变量:"
    echo "  SWITCH_IP=192.168.x.x    Switch 的 IP 地址（deploy 时需要）"
    echo "  JOBS=4                   并行编译数（默认 4）"
    echo ""
    echo "示例:"
    echo "  # 首次部署环境"
    echo "  ./scripts/dev_switch.sh setup"
    echo ""
    echo "  # 日常开发编译"
    echo "  ./scripts/dev_switch.sh build"
    echo ""
    echo "  # 编译并部署到 Switch"
    echo "  SWITCH_IP=192.168.1.100 ./scripts/dev_switch.sh build"
    echo "  SWITCH_IP=192.168.1.100 ./scripts/dev_switch.sh deploy"
    echo ""
    echo "  # 一键完成所有步骤"
    echo "  SWITCH_IP=192.168.1.100 ./scripts/dev_switch.sh all"
}

# 主入口
main() {
    case "${1:-help}" in
        setup)
            check_devkitpro
            do_setup
            ;;
        build)
            check_devkitpro
            do_build
            ;;
        rebuild)
            check_devkitpro
            do_rebuild
            ;;
        deploy)
            do_deploy
            ;;
        all)
            check_devkitpro
            do_setup
            do_build
            if [ -n "${SWITCH_IP}" ]; then
                do_deploy
            else
                log_warn "未设置 SWITCH_IP，跳过部署"
            fi
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            log_error "未知命令: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
