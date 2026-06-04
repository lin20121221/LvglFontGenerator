#!/bin/bash

echo "========================================"
echo "LvglFontGenerator 自动编译和部署"
echo "========================================"
echo ""

# 配置路径
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

# 检查 CMake 是否可用
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] CMake 未找到，请先安装 CMake"
    exit 1
fi

# 检查 Qt 是否可用
if ! command -v qmake &> /dev/null; then
    echo "[ERROR] Qt 未找到，请确保 Qt 已安装并在 PATH 中"
    exit 1
fi

# 检查 HarfBuzz 是否可用
if ! pkg-config --exists harfbuzz; then
    echo "[WARNING] HarfBuzz 未找到，请安装 HarfBuzz"
    echo "  Ubuntu/Debian: sudo apt-get install libharfbuzz-dev"
    echo "  Fedora: sudo dnf install harfbuzz-devel"
    echo "  macOS: brew install harfbuzz"
fi

echo "[1/4] 清理旧的构建目录..."
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
echo "[OK] 构建目录已清理"

echo ""
echo "[2/4] 配置 CMake..."
cd "$BUILD_DIR"
cmake "$PROJECT_DIR"
if [ $? -ne 0 ]; then
    echo "[ERROR] CMake 配置失败"
    exit 1
fi
echo "[OK] CMake 配置成功"

echo ""
echo "[3/4] 编译项目..."
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo "[ERROR] 编译失败"
    exit 1
fi
echo "[OK] 编译成功"

echo ""
echo "[4/4] 检查可执行文件..."
if [ -f "$BUILD_DIR/LvglFontGenerator" ]; then
    echo "[OK] 可执行文件已生成: $BUILD_DIR/LvglFontGenerator"
    chmod +x "$BUILD_DIR/LvglFontGenerator"
else
    echo "[ERROR] 可执行文件未找到"
    exit 1
fi

echo ""
echo "========================================"
echo "编译和部署完成！"
echo "========================================"
echo ""
echo "可执行文件: $BUILD_DIR/LvglFontGenerator"
echo ""

# 询问是否运行
read -p "是否现在运行程序? (y/n) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    "$BUILD_DIR/LvglFontGenerator" &
    echo ""
    echo "程序已启动！"
fi

echo ""
