# 第三方 DLL 部署指南

## 问题

编译后的 LvglFontGenerator.exe 需要多个第三方 DLL 才能运行。如果缺少这些 DLL，程序会报错或无法启动。

## 快速解决方案

### 方法 1: 使用自动部署脚本（推荐）

```batch
双击运行: build_and_deploy.bat
```

这会自动：
- 编译程序
- 部署 Qt DLL（使用 windeployqt）
- 复制所有 MSYS2/MinGW DLL

### 方法 2: 手动复制 DLL

```batch
双击运行: copy_dlls.bat
```

这会复制所有必需的第三方 DLL 到 Release 和 Debug 目录。

### 方法 3: 检查依赖

```batch
双击运行: check_dependencies.bat
```

这会：
- 测试程序是否能启动
- 列出当前的 DLL 文件
- 显示必需的 DLL 清单

## 必需的 DLL 清单

### Qt 依赖（由 windeployqt 自动部署）

```
Qt6Core.dll
Qt6Gui.dll
Qt6Widgets.dll
Qt6Network.dll (可选)
Qt6Svg.dll (可选)
Qt6Pdf.dll (可选)
```

### HarfBuzz 依赖（从 MSYS2 复制）

```
libharfbuzz-0.dll       # HarfBuzz 主库
libgraphite2.dll        # Graphite2 字体引擎
```

### GLib 依赖（从 MSYS2 复制）

```
libglib-2.0-0.dll       # GLib 核心库
libintl-8.dll           # 国际化支持
libpcre2-8-0.dll        # 正则表达式库
libiconv-2.dll          # 字符编码转换
```

### MinGW 运行时（从 MSYS2 复制）

```
libgcc_s_seh-1.dll      # GCC 运行时
libstdc++-6.dll         # C++ 标准库
libwinpthread-1.dll     # POSIX 线程库
```

### FreeType 依赖（从 MSYS2 复制）

```
libfreetype-6.dll       # FreeType 字体渲染库
libbz2-1.dll            # BZip2 压缩库
libpng16-16.dll         # PNG 图像库
zlib1.dll               # Zlib 压缩库
libbrotlicommon.dll     # Brotli 压缩库（通用）
libbrotlidec.dll        # Brotli 压缩库（解压）
```

## DLL 来源

### Qt DLL

**位置：** `C:\Qt\6.11.0\msvc2022_64\bin\`

**部署方法：**
```batch
cd build\Release
C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe LvglFontGenerator.exe
```

### MSYS2/MinGW DLL

**位置：** `C:\msys64\mingw64\bin\`

**部署方法：**
```batch
copy C:\msys64\mingw64\bin\libharfbuzz-0.dll build\Release\
copy C:\msys64\mingw64\bin\libgraphite2.dll build\Release\
copy C:\msys64\mingw64\bin\libglib-2.0-0.dll build\Release\
REM ... 复制其他 DLL
```

或使用 `copy_dlls.bat` 自动复制所有 DLL。

## 故障排除

### 问题 1: 程序无法启动，提示缺少 DLL

**症状：**
```
The code execution cannot proceed because XXX.dll was not found.
```

**解决方法：**
1. 运行 `copy_dlls.bat` 复制所有 DLL
2. 或手动复制缺失的 DLL 到可执行文件目录

### 问题 2: 程序启动后立即崩溃

**可能原因：**
- DLL 版本不匹配
- 混用了不同编译器的 DLL（MSVC vs MinGW）

**解决方法：**
1. 确保所有 MinGW DLL 都来自同一个 MSYS2 安装
2. 删除 build 目录，重新运行 `build_and_deploy.bat`

### 问题 3: 不知道缺少哪个 DLL

**解决方法：**

**方法 A: 使用检查脚本**
```batch
check_dependencies.bat
```

**方法 B: 使用 Dependency Walker**
1. 下载 Dependency Walker: https://www.dependencywalker.com/
2. 打开 LvglFontGenerator.exe
3. 查看红色标记的缺失 DLL

**方法 C: 查看 Windows 事件日志**
1. 打开"事件查看器"
2. Windows 日志 → 应用程序
3. 查找 LvglFontGenerator 相关的错误

### 问题 4: MSYS2 未安装

**解决方法：**

1. 下载 MSYS2: https://www.msys2.org/
2. 安装到 `C:\msys64`
3. 打开 MSYS2 MinGW 64-bit 终端
4. 安装 HarfBuzz:
   ```bash
   pacman -S mingw-w64-x86_64-harfbuzz
   ```

## 发布打包

如果要发布 LvglFontGenerator 给其他用户，需要打包所有 DLL：

### 步骤 1: 完整编译和部署

```batch
build_and_deploy.bat
```

### 步骤 2: 验证程序可以运行

```batch
cd build\Release
LvglFontGenerator.exe
```

### 步骤 3: 打包发布

将整个 `build\Release\` 目录打包为 ZIP：

```
LvglFontGenerator-v2.0.0-win64.zip
├── LvglFontGenerator.exe
├── Qt6Core.dll
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── libharfbuzz-0.dll
├── libgraphite2.dll
├── ... (所有其他 DLL)
├── platforms\
│   └── qwindows.dll
├── styles\
│   └── qwindowsvistastyle.dll
└── README.txt (使用说明)
```

### 步骤 4: 测试打包

在另一台没有安装 Qt 和 MSYS2 的电脑上测试：
1. 解压 ZIP 文件
2. 双击 LvglFontGenerator.exe
3. 确认程序正常启动

## DLL 大小参考

总大小约 **50-80 MB**（包含所有 DLL）

主要 DLL 大小：
- Qt6Core.dll: ~11 MB
- Qt6Gui.dll: ~9 MB
- Qt6Widgets.dll: ~6 MB
- libharfbuzz-0.dll: ~1.5 MB
- libfreetype-6.dll: ~1 MB
- 其他 DLL: 各 100-500 KB

## 自动化脚本说明

### build_and_deploy.bat

**功能：**
- 清理构建目录
- 配置 CMake
- 编译 Release 和 Debug
- 部署 Qt DLL
- 复制 MSYS2 DLL

**使用：**
```batch
双击运行或在命令行执行
```

### copy_dlls.bat

**功能：**
- 只复制 MSYS2/MinGW DLL
- 不重新编译
- 适合 DLL 更新后使用

**使用：**
```batch
双击运行
```

### check_dependencies.bat

**功能：**
- 测试程序是否能启动
- 列出当前 DLL
- 显示必需 DLL 清单

**使用：**
```batch
双击运行
```

## 常见问题

### Q: 为什么需要这么多 DLL？

A: 因为使用了多个第三方库：
- Qt: 跨平台 GUI 框架
- HarfBuzz: OpenType 文本 shaping
- FreeType: 字体渲染
- GLib: 通用工具库

### Q: 可以静态链接吗？

A: 理论上可以，但：
- Qt 商业许可证才支持静态链接
- 静态链接会增加可执行文件大小
- 动态链接更灵活，便于更新

### Q: 可以减少 DLL 数量吗？

A: 部分可以：
- Qt6Network.dll, Qt6Svg.dll, Qt6Pdf.dll 可能不是必需的
- 但核心 DLL（Qt6Core, Qt6Gui, Qt6Widgets, HarfBuzz 等）是必需的

### Q: Linux/macOS 需要这些 DLL 吗？

A: 不需要。Linux/macOS 使用系统的共享库（.so / .dylib），通常已经安装。

## 总结

- ✅ 使用 `build_and_deploy.bat` 一键编译和部署
- ✅ 使用 `copy_dlls.bat` 快速复制 DLL
- ✅ 使用 `check_dependencies.bat` 检查依赖
- ✅ 发布时打包整个 Release 目录

**所有脚本都已配置好，开箱即用！** 🚀
