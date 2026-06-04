# HarfBuzz 安装指南

## Windows 安装方法

### 方法 1: 使用 vcpkg (推荐)

```bash
# 安装 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装 HarfBuzz
.\vcpkg install harfbuzz:x64-windows

# 集成到 CMake
.\vcpkg integrate install
```

然后在 CMake 配置时添加：
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```

### 方法 2: 使用 MSYS2

```bash
# 打开 MSYS2 MinGW 64-bit 终端
pacman -S mingw-w64-x86_64-harfbuzz
```

### 方法 3: 手动下载预编译库

从 https://github.com/harfbuzz/harfbuzz/releases 下载预编译的 Windows 版本。

## Linux 安装方法

### Ubuntu/Debian
```bash
sudo apt-get install libharfbuzz-dev
```

### Fedora/RHEL
```bash
sudo dnf install harfbuzz-devel
```

### Arch Linux
```bash
sudo pacman -S harfbuzz
```

## macOS 安装方法

```bash
brew install harfbuzz
```

## 验证安装

```bash
pkg-config --modversion harfbuzz
```

应该输出 HarfBuzz 的版本号（例如：8.3.0）
