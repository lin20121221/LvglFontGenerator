# FreeType 库安装指南

本项目需要 FreeType 库来渲染字体。以下是在 Windows 上安装 FreeType 的方法。

## 方法 1: 使用 vcpkg (推荐)

### 1. 安装 vcpkg (如果还没有安装)

```bash
# 克隆 vcpkg 仓库到某个目录
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# 运行 bootstrap 脚本
.\bootstrap-vcpkg.bat

# (可选) 将 vcpkg 添加到系统环境变量
setx PATH "%PATH%;C:\vcpkg"
```

### 2. 安装 FreeType

```bash
# 使用 vcpkg 安装 freetype
vcpkg install freetype:x64-windows

# 如果使用 32 位
vcpkg install freetype:x86-windows
```

### 3. 配置 CMake 使用 vcpkg

在 Qt Creator 或命令行中配置 CMake 时，添加以下参数：

```bash
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

或者在 Qt Creator 中：
1. 打开 **工具 -> 选项 -> Kits -> CMake**
2. 在 CMake 配置中添加：`CMAKE_TOOLCHAIN_FILE:STRING=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`

## 方法 2: 手动下载预编译库

### 1. 下载 FreeType

从以下地址下载预编译的 FreeType 库：
- https://github.com/ubawurinna/freetype-windows-binaries

### 2. 配置环境变量

将 FreeType 的路径添加到 CMake 配置中：

```bash
-DFREETYPE_INCLUDE_DIRS=C:/path/to/freetype/include
-DFREETYPE_LIBRARIES=C:/path/to/freetype/lib/freetype.lib
```

## 方法 3: 使用 MSYS2/MinGW

如果你使用 MSYS2 环境：

```bash
# 打开 MSYS2 终端
pacman -S mingw-w64-x86_64-freetype

# 32 位版本
pacman -S mingw-w64-i686-freetype
```

## 验证安装

重新配置并构建项目：

```bash
cd build
cmake ..
cmake --build .
```

如果配置成功，你应该看到类似以下的输出：
```
-- FreeType found: /path/to/freetype.lib
-- FreeType include: /path/to/freetype/include
```

## 故障排除

### CMake 找不到 FreeType

如果 CMake 无法找到 FreeType，尝试：

1. 确认 FreeType 已正确安装
2. 检查 CMake 工具链文件是否正确配置
3. 手动指定 FreeType 路径：
   ```bash
   cmake -DFREETYPE_INCLUDE_DIRS=<path> -DFREETYPE_LIBRARIES=<path> ..
   ```

### 链接错误

如果出现链接错误，确保：
- 使用的 FreeType 库与编译器匹配（MinGW vs MSVC）
- 架构匹配（x64 vs x86）
