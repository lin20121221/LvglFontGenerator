# LvglFontGenerator 自动编译和部署指南

## 概述

提供了自动化脚本来简化 LvglFontGenerator 的编译和部署过程。

## Windows 平台

### 方法 1: 完整编译和部署（推荐首次使用）

```batch
双击运行: build_and_deploy.bat
```

**功能：**
- 清理旧的构建目录
- 配置 CMake
- 编译 Release 和 Debug 版本
- 自动部署 Qt 依赖（使用 windeployqt）
- 复制 HarfBuzz DLL
- 可选：启动程序

**前提条件：**
- CMake 已安装并在 PATH 中
- Visual Studio 2022 已安装
- Qt 6.11.0 (MSVC 2022 64-bit) 已安装
- MSYS2 已安装（用于 HarfBuzz DLL）

**配置路径：**

如果你的安装路径不同，请编辑 `build_and_deploy.bat` 修改以下变量：

```batch
set "QT_DIR=C:\Qt\6.11.0\msvc2022_64"
set "CMAKE_GENERATOR=Visual Studio 17 2022"
set "MSYS2_BIN=C:\msys64\mingw64\bin"
```

### 方法 2: 快速重新编译

```batch
双击运行: rebuild.bat
```

**功能：**
- 快速重新编译 Release 和 Debug 版本
- 不清理构建目录
- 不重新部署依赖
- 适合代码修改后的快速编译

**使用场景：**
- 修改了源代码
- 已经运行过完整编译
- 只需要重新编译，不需要重新部署依赖

### 方法 3: 从 Qt Creator 编译

1. 用 Qt Creator 打开 `CMakeLists.txt`
2. 配置项目（选择 MSVC 2022 64-bit kit）
3. 点击 "Build" → "Build All"
4. 运行程序

**优点：**
- 集成开发环境
- 方便调试
- 自动处理依赖

**注意：**
- 需要手动复制 HarfBuzz DLL 到构建目录
- 或者运行一次 `build_and_deploy.bat` 来部署 DLL

---

## Linux 平台

### 完整编译和部署

```bash
./build_and_deploy.sh
```

**功能：**
- 清理旧的构建目录
- 配置 CMake
- 编译项目（使用多核并行编译）
- 检查可执行文件
- 可选：启动程序

**前提条件：**

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install qt6-base-dev qt6-tools-dev
sudo apt-get install libharfbuzz-dev libfreetype-dev
```

**Fedora:**
```bash
sudo dnf install gcc-c++ cmake
sudo dnf install qt6-qtbase-devel qt6-qttools-devel
sudo dnf install harfbuzz-devel freetype-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake
sudo pacman -S qt6-base qt6-tools
sudo pacman -S harfbuzz freetype2
```

### 快速重新编译

```bash
cd build
cmake --build . -j$(nproc)
```

---

## macOS 平台

### 完整编译和部署

```bash
./build_and_deploy.sh
```

**前提条件：**

```bash
# 安装 Homebrew（如果还没有）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake
brew install qt@6
brew install harfbuzz freetype

# 添加 Qt 到 PATH
echo 'export PATH="/usr/local/opt/qt@6/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

---

## 手动编译（所有平台）

如果自动脚本不适用，可以手动编译：

### 步骤 1: 创建构建目录

```bash
mkdir build
cd build
```

### 步骤 2: 配置 CMake

**Windows (MSVC):**
```batch
cmake -G "Visual Studio 17 2022" -A x64 ..
```

**Windows (MinGW):**
```batch
cmake -G "MinGW Makefiles" ..
```

**Linux/macOS:**
```bash
cmake ..
```

### 步骤 3: 编译

**Windows (MSVC):**
```batch
cmake --build . --config Release
cmake --build . --config Debug
```

**Linux/macOS:**
```bash
cmake --build . -j$(nproc)
```

### 步骤 4: 部署依赖（Windows）

**部署 Qt 依赖:**
```batch
cd Release
C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe LvglFontGenerator.exe
```

**复制 HarfBuzz DLL:**
```batch
copy C:\msys64\mingw64\bin\libharfbuzz-0.dll .
copy C:\msys64\mingw64\bin\libgraphite2.dll .
copy C:\msys64\mingw64\bin\libglib-2.0-0.dll .
copy C:\msys64\mingw64\bin\libintl-8.dll .
copy C:\msys64\mingw64\bin\libpcre2-8-0.dll .
```

---

## 构建输出

### Windows

```
build/
├── Release/
│   ├── LvglFontGenerator.exe    # Release 可执行文件
│   ├── Qt6Core.dll               # Qt 依赖
│   ├── Qt6Gui.dll
│   ├── Qt6Widgets.dll
│   ├── libharfbuzz-0.dll         # HarfBuzz 依赖
│   └── ...                       # 其他 DLL
└── Debug/
    ├── LvglFontGenerator.exe    # Debug 可执行文件
    └── ...                       # 依赖 DLL
```

### Linux/macOS

```
build/
└── LvglFontGenerator            # 可执行文件
```

---

## 故障排除

### Windows

**问题：CMake 未找到**
```
解决：安装 CMake 并添加到 PATH
下载：https://cmake.org/download/
```

**问题：Visual Studio 未找到**
```
解决：安装 Visual Studio 2022 Community
下载：https://visualstudio.microsoft.com/
确保安装 "Desktop development with C++" 工作负载
```

**问题：Qt 未找到**
```
解决：安装 Qt 6.11.0 (MSVC 2022 64-bit)
下载：https://www.qt.io/download-qt-installer
```

**问题：HarfBuzz DLL 未找到**
```
解决：安装 MSYS2 并安装 HarfBuzz
下载：https://www.msys2.org/
在 MSYS2 终端运行：pacman -S mingw-w64-x86_64-harfbuzz
```

**问题：程序无法启动，提示缺少 DLL**
```
解决：运行 build_and_deploy.bat 重新部署依赖
或手动复制缺失的 DLL 到可执行文件目录
```

### Linux

**问题：Qt 未找到**
```
解决：安装 Qt 开发包
Ubuntu: sudo apt-get install qt6-base-dev
Fedora: sudo dnf install qt6-qtbase-devel
```

**问题：HarfBuzz 未找到**
```
解决：安装 HarfBuzz 开发包
Ubuntu: sudo apt-get install libharfbuzz-dev
Fedora: sudo dnf install harfbuzz-devel
```

### macOS

**问题：Qt 未找到**
```
解决：使用 Homebrew 安装 Qt
brew install qt@6
export PATH="/usr/local/opt/qt@6/bin:$PATH"
```

**问题：HarfBuzz 未找到**
```
解决：使用 Homebrew 安装 HarfBuzz
brew install harfbuzz
```

---

## 开发工作流

### 日常开发

1. **修改代码**
2. **快速编译：** 运行 `rebuild.bat` (Windows) 或 `cd build && cmake --build .` (Linux/macOS)
3. **测试**
4. **提交代码**

### 重大更改后

1. **完整重新编译：** 运行 `build_and_deploy.bat` (Windows) 或 `./build_and_deploy.sh` (Linux/macOS)
2. **测试所有功能**
3. **提交代码**

### 发布版本

1. **运行完整编译：** `build_and_deploy.bat`
2. **测试 Release 版本**
3. **打包发布：** 将 `build/Release/` 目录打包为 ZIP
4. **发布到 GitHub Releases**

---

## 持续集成（CI）

可以使用 GitHub Actions 自动编译：

创建 `.github/workflows/build.yml`:

```yaml
name: Build

on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - uses: jurplel/install-qt-action@v3
        with:
          version: '6.11.0'
      - name: Build
        run: |
          mkdir build
          cd build
          cmake -G "Visual Studio 17 2022" -A x64 ..
          cmake --build . --config Release

  build-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y qt6-base-dev libharfbuzz-dev
      - name: Build
        run: |
          mkdir build
          cd build
          cmake ..
          cmake --build .
```

---

## 总结

- **Windows 用户：** 使用 `build_and_deploy.bat` 一键编译和部署
- **Linux/macOS 用户：** 使用 `./build_and_deploy.sh` 一键编译
- **开发者：** 使用 Qt Creator 或 `rebuild.bat` 快速编译
- **CI/CD：** 使用 GitHub Actions 自动编译

所有脚本都已配置好，开箱即用！🚀
