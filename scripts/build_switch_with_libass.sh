#!/bin/bash
set -e

BUILD_DIR=cmake-build-switch
SCRIPT_DIR="$(cd "$(dirname $0)" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"
git config --global --add safe.directory "$PROJECT_DIR" 2>/dev/null || true

echo "=========================================="
echo "编译带 libass 字幕支持的 Switch 版本"
echo "=========================================="
echo ""
echo "注意：由于 deko3d 版本的 MPV patch 不可用，"
echo "此脚本将使用 OpenGL 版本的 MPV（非 deko3d）"
echo "=========================================="

# 设置 devkitPro 环境变量
export DEVKITPRO=/opt/devkitpro
export PATH=$DEVKITPRO/tools/bin:$DEVKITPRO/devkitA64/bin:$PATH
export PORTLIBS_PREFIX=$DEVKITPRO/portlibs/switch
export PKG_CONFIG_LIBDIR=$PORTLIBS_PREFIX/lib/pkgconfig

# 检测操作系统
OS_TYPE=$(uname -s)
echo "检测到操作系统: $OS_TYPE"

# 获取 CPU 核心数
if [ "$OS_TYPE" = "Darwin" ]; then
    NPROC=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
else
    NPROC=$(nproc 2>/dev/null || echo 4)
fi

# 安装必需的系统包
echo "安装必需的系统包..."
dkp-pacman -Sy --noconfirm
dkp-pacman -S --noconfirm --needed switch-dev switch-curl switch-libplacebo switch-freetype switch-libfribidi switch-liblua51 switch-mesa switch-sdl2 dkp-meson-scripts dkp-toolchain-vars

# 安装 meson 和 ninja（如果不存在）
if ! command -v meson &> /dev/null; then
    echo "安装 meson 和 ninja..."
    if [ "$OS_TYPE" = "Darwin" ]; then
        # macOS 使用 Homebrew
        if command -v brew &> /dev/null; then
            brew install meson ninja
        else
            echo "错误：未找到 Homebrew，请先安装 meson：brew install meson"
            exit 1
        fi
    else
        # Linux 使用 apt-get
        apt-get update && apt-get install -y meson ninja-build
    fi
fi

# 安装 libuam
BASE_URL="https://github.com/xfangfang/wiliwili/releases/download/v0.1.0/"
LIBUAM_PKG="libuam-f8c9eef01ffe06334d530393d636d69e2b52744b-1-any.pkg.tar.zst"
[ -f "${LIBUAM_PKG}" ] || curl -LO ${BASE_URL}${LIBUAM_PKG}
dkp-pacman -U --noconfirm ${LIBUAM_PKG} || true

# ==========================================
# 编译 harfbuzz
# ==========================================
echo "检查 harfbuzz..."
if [ ! -f "$PORTLIBS_PREFIX/lib/libharfbuzz.a" ]; then
    echo "编译 harfbuzz..."
    HARFBUZZ_VER=7.1.0
    WORK_DIR=$(mktemp -d)
    cd "$WORK_DIR"
    curl -LO https://github.com/harfbuzz/harfbuzz/releases/download/$HARFBUZZ_VER/harfbuzz-$HARFBUZZ_VER.tar.xz
    tar xf harfbuzz-$HARFBUZZ_VER.tar.xz
    cd harfbuzz-$HARFBUZZ_VER
    mkdir -p build && cd build
    
    aarch64-none-elf-cmake -G"Unix Makefiles" \
        -DCMAKE_INSTALL_PREFIX="$PORTLIBS_PREFIX" \
        -DHB_HAVE_FREETYPE=ON \
        ..
    make -j$NPROC
    make install
    cd "$PROJECT_DIR"
    rm -rf "$WORK_DIR"
else
    echo "harfbuzz 已安装，跳过"
fi

# ==========================================
# 编译 libass
# ==========================================
echo "检查 libass..."
if [ ! -f "$PORTLIBS_PREFIX/lib/libass.a" ]; then
    echo "编译 libass..."
    LIBASS_VER=0.17.1
    WORK_DIR=$(mktemp -d)
    cd "$WORK_DIR"
    curl -LO https://github.com/libass/libass/releases/download/$LIBASS_VER/libass-$LIBASS_VER.tar.xz
    tar xf libass-$LIBASS_VER.tar.xz
    cd libass-$LIBASS_VER
    
    # 设置交叉编译环境
    export CC="aarch64-none-elf-gcc"
    export CXX="aarch64-none-elf-g++"
    export AR="aarch64-none-elf-ar"
    export RANLIB="aarch64-none-elf-ranlib"
    export CFLAGS="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec -O2"
    export CXXFLAGS="$CFLAGS"
    export LDFLAGS="-L$PORTLIBS_PREFIX/lib -L$DEVKITPRO/libnx/lib"
    export PKG_CONFIG_PATH="$PORTLIBS_PREFIX/lib/pkgconfig"
    
    ./configure --prefix="$PORTLIBS_PREFIX" --host=aarch64-none-elf \
        --disable-shared --enable-static \
        --disable-asm --enable-large-tiles \
        --disable-require-system-font-provider
    make -j$NPROC
    make install
    cd "$PROJECT_DIR"
    rm -rf "$WORK_DIR"
else
    echo "libass 已安装，跳过"
fi

# ==========================================
# 编译带 libass 的 mpv (使用 meson)
# ==========================================
echo "编译带 libass 支持的 mpv..."
MPV_VER=0.36.0
WORK_DIR=$(mktemp -d)
cd "$WORK_DIR"

# 检查本地是否已有源码包
if [ -f "$PROJECT_DIR/v$MPV_VER.tar.gz" ]; then
    echo "使用本地 mpv 源码包..."
    cp "$PROJECT_DIR/v$MPV_VER.tar.gz" .
else
    echo "下载 mpv 源码..."
    curl -LO --retry 3 --retry-delay 5 https://github.com/mpv-player/mpv/archive/v$MPV_VER.tar.gz
fi
tar xf v$MPV_VER.tar.gz
cd mpv-$MPV_VER

# 应用 patch
if [ -f "$PROJECT_DIR/scripts/switch/mpv/mpv.patch" ]; then
    echo "应用 mpv.patch..."
    rm -rf audio/out/ao_hos.c
    rm -rf osdep/switch/sys/mman.h
    patch -Np1 -i "$PROJECT_DIR/scripts/switch/mpv/mpv.patch"
fi

# 使用 meson 构建
# 注意：libass 会自动检测，不需要显式启用
echo "配置 meson..."
$DEVKITPRO/meson-cross.sh switch crossfile.txt build \
    -Dlua=enabled \
    -Dhos=enabled \
    -Dhos-audio=enabled \
    -Diconv=disabled \
    -Djpeg=disabled \
    -Dlibavdevice=disabled \
    -Dmanpage-build=disabled \
    -Dsdl2=disabled \
    -Dlibmpv=true \
    -Dcplayer=false

echo "编译 mpv..."
meson compile -C build -j$NPROC
meson install -C build --destdir="$PORTLIBS_PREFIX/.."

cd "$PROJECT_DIR"
rm -rf "$WORK_DIR"

# ==========================================
# 安装其他必需的包
# ==========================================
PKGS=(
    "switch-ffmpeg-7.1-1-any.pkg.tar.zst"
    "switch-nspmini-48d4fc2-1-any.pkg.tar.xz"
    "hacBrewPack-3.05-1-any.pkg.tar.zst"
)
for PKG in "${PKGS[@]}"; do
    [ -f "${PKG}" ] || curl -LO ${BASE_URL}${PKG}
    dkp-pacman -U --noconfirm ${PKG} || true
done

# ==========================================
# 编译 wiliwili (使用 OpenGL 而非 deko3d)
# ==========================================
echo "PKG_CONFIG_LIBDIR 设置为: $PKG_CONFIG_LIBDIR"

# 清理旧的编译目录
rm -rf ${BUILD_DIR}

echo "编译 wiliwili (OpenGL 版本)..."

# 检测可用内存，决定编译策略
# Docker 容器内存不足时会导致编译器被 kill
AVAILABLE_MEM=0
if [ -f /proc/meminfo ]; then
    AVAILABLE_MEM=$(grep MemAvailable /proc/meminfo | awk '{print int($2/1024/1024)}')
fi

echo "可用内存: ${AVAILABLE_MEM}GB"

# 根据内存调整编译参数
if [ "$AVAILABLE_MEM" -lt 6 ]; then
    echo "警告：内存不足 6GB，禁用 Unity Build 并使用单线程编译"
    UNITY_BUILD=OFF
    BUILD_JOBS=1
elif [ "$AVAILABLE_MEM" -lt 10 ]; then
    echo "内存有限，减少 Unity Build batch size 和并行任务数"
    UNITY_BUILD=ON
    UNITY_BATCH_SIZE=4
    BUILD_JOBS=2
else
    echo "内存充足，使用完整编译配置"
    UNITY_BUILD=ON
    UNITY_BATCH_SIZE=16
    BUILD_JOBS=$NPROC
fi

# 设置默认值（如果未设置）
UNITY_BUILD=${UNITY_BUILD:-ON}
UNITY_BATCH_SIZE=${UNITY_BATCH_SIZE:-16}
BUILD_JOBS=${BUILD_JOBS:-$NPROC}

# 允许通过环境变量覆盖
if [ -n "$FORCE_JOBS" ]; then
    BUILD_JOBS=$FORCE_JOBS
    echo "使用环境变量指定的并行任务数: $BUILD_JOBS"
fi

if [ "$FORCE_NO_UNITY" = "1" ]; then
    UNITY_BUILD=OFF
    echo "通过环境变量禁用 Unity Build"
fi

echo "编译配置: Unity Build=$UNITY_BUILD, Batch Size=$UNITY_BATCH_SIZE, Jobs=$BUILD_JOBS"

# 注意：不使用 USE_DEKO3D，使用 SDL2 + OpenGL 渲染
# 必须设置 USE_SDL2=ON，否则会回退到 GLFW（Switch 上不可用）
# 禁用 Unity Build 以避免 lunasvg 符号重定义问题
cmake -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=Release \
    -DBUILTIN_NSP=OFF \
    -DPLATFORM_SWITCH=ON \
    -DUSE_SDL2=ON \
    -DBRLS_UNITY_BUILD=OFF \
    -DCMAKE_UNITY_BUILD=OFF

make -C ${BUILD_DIR} wiliwili.nro -j$BUILD_JOBS

echo "=========================================="
echo "编译完成！"
echo "输出文件: ${BUILD_DIR}/wiliwili.nro"
echo "=========================================="
