#!/bin/bash
# 把 build 目录下新编译的 CarClean 快速同步到 Luckfox SDK 的 overlay-carclean 目录,
# 之后在 SDK 根目录执行 ./build.sh 即可把新程序打进镜像。
#
# 用法: ./sync_to_overlay.sh

BUILD_BIN="/home/wjc/CarClean/build/CarClean"
OVERLAY_DIR="/home/wjc/luckfox-pico/project/cfg/BoardConfig_IPC/overlay/overlay-carclean"

set -e

if [ ! -f "$BUILD_BIN" ]; then
    echo "错误: 找不到 $BUILD_BIN"
    echo "      请先在 /home/wjc/CarClean 完成构建 (./build.sh)"
    exit 1
fi

if [ ! -d "$OVERLAY_DIR" ]; then
    echo "错误: overlay 目录不存在: $OVERLAY_DIR"
    exit 1
fi

echo "同步 CarClean -> overlay-carclean ..."
cp -v "$BUILD_BIN" "$OVERLAY_DIR/CarClean"
chmod +x "$OVERLAY_DIR/CarClean"
ls -la "$OVERLAY_DIR/CarClean"
echo ""
echo "完成。下一步: cd /home/wjc/luckfox-pico && ./build.sh 打镜像"
