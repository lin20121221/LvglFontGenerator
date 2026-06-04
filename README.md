# LvglFontGenerator - HarfBuzz 集成版本

## 概述

LvglFontGenerator 是一个用于生成 LVGL 字体文件的工具，现已集成 **HarfBuzz** 库，完全支持现代 OpenType 字体的 kerning 特性。

## 主要特性

### ✨ 新增功能

- ✅ **完整的 GPOS Kerning 支持** - 使用 HarfBuzz shaping 引擎
- ✅ **实时预览 Kerning 效果** - 预览窗口正确显示字符间距调整
- ✅ **智能回退机制** - HarfBuzz 失败时自动回退到 FreeType
- ✅ **性能优化** - Kerning 缓存机制提高预览流畅度
- ✅ **自动编译部署** - 一键编译和部署脚本

### 🔧 技术改进

**Kerning 提取：**
- 支持 GPOS LookupType 2 (PairPos)
- 支持 GPOS LookupType 9 (ExtensionPos)
- 支持传统 kern 表
- 使用 HarfBuzz shaping 获取准确的字符间距

**预览功能：**
- 集成 HarfBuzz 到 FreeTypeRenderer
- 实时显示 kerning 效果
- 缓存机制避免重复计算

**导出功能：**
- 生成完整的 kerning 类映射表
- 优化的 kerning 数据结构
- 兼容 LVGL 8.x 和 9.x

## 支持的字体

| 字体类型 | kern 表 | GPOS 表 | 支持状态 |
|---------|---------|---------|---------|
| Arial, Times New Roman | ✓ | ✓ | ✅ 完全支持 |
| MyriadPro, Source Sans | ✗ | ✓ | ✅ 完全支持 |
| 旧式 TrueType | ✓ | ✗ | ✅ 完全支持 |

**测试通过的字体：**
- Arial Regular / Bold
- MyriadPro Bold
- 以及其他包含 kerning 数据的字体

## 快速开始

### Windows

**方法 1: 一键编译和部署**
```batch
双击运行: build_and_deploy.bat
```

**方法 2: 快速重新编译**
```batch
双击运行: rebuild.bat
```

**方法 3: 使用 Qt Creator**
1. 打开 `CMakeLists.txt`
2. 配置项目
3. 编译运行

### Linux/macOS

```bash
./build_and_deploy.sh
```

详细说明请参考 [BUILD_DEPLOY_GUIDE.md](BUILD_DEPLOY_GUIDE.md)

## 使用说明

### 生成带 Kerning 的字体

1. **启动程序**
   ```
   build/Release/LvglFontGenerator.exe  (Windows)
   build/LvglFontGenerator              (Linux/macOS)
   ```

2. **选择字体文件**
   - 点击 "Browse" 选择 TTF/OTF 字体文件

3. **配置参数**
   - 字号：16 (或其他)
   - BPP：4 或 8 (推荐 8)
   - **✓ 勾选 "Enable Kerning"** (重要！)

4. **输入字符**
   - 输入需要的字符，例如：`ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789`
   - 或使用 Unicode 格式：`U+0041 U+0042`

5. **预览效果**
   - 在预览窗口中查看 kerning 效果
   - 勾选/取消 "Enable Kerning" 对比差异

6. **生成字体**
   - 点击 "Generate" 生成 .c 文件
   - 检查输出文件中的 kerning 表

### 验证 Kerning 数据

生成的 .c 文件应该包含：

```c
/*--------------------
 *  KERNING
 *-------------------*/

/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 1, 2, 3, ...
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 2, 3, ...
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, -19, -21, -14, ...
};
```

如果看到 `/* No kerning data */`，说明：
- 没有勾选 "Enable Kerning"
- 字体不包含 kerning 数据
- 或者使用了旧版本（未集成 HarfBuzz）

## 依赖项

### 编译时依赖

- **CMake** 3.16+
- **Qt** 6.11.0+ (Core, Gui, Widgets)
- **HarfBuzz** (开发库)
- **FreeType** (通常由 Qt 提供)

### 运行时依赖 (Windows)

- Qt6Core.dll
- Qt6Gui.dll
- Qt6Widgets.dll
- libharfbuzz-0.dll
- libgraphite2.dll
- libglib-2.0-0.dll
- libintl-8.dll
- libpcre2-8-0.dll

**注意：** 使用 `build_and_deploy.bat` 会自动复制所有依赖。

## 项目结构

```
LvglFontGenerator/
├── src/
│   ├── main.cpp
│   ├── mainwindow.cpp/h
│   ├── fontgenerator.cpp/h
│   ├── lvglexporter.cpp/h
│   ├── fontpreviewwidget.cpp/h
│   ├── freetyperenderer.cpp/h      # 集成 HarfBuzz
│   ├── harfbuzzkerning.cpp/h       # HarfBuzz kerning 提取器
│   ├── opentypekerning.cpp/h       # FreeType kerning 提取器（回退）
│   └── kerningoptimizer.cpp/h
├── CMakeLists.txt
├── build_and_deploy.bat            # Windows 自动编译脚本
├── rebuild.bat                     # Windows 快速重新编译
├── build_and_deploy.sh             # Linux/macOS 编译脚本
└── 文档/
    ├── BUILD_DEPLOY_GUIDE.md       # 编译部署指南
    ├── KERNING_FIX_SUMMARY.md      # Kerning 修复总结
    ├── FONT_KERNING_TEST_REPORT.md # 字体测试报告
    ├── PREVIEW_KERNING_FIXED.md    # 预览修复说明
    └── HARFBUZZ_INSTALL.md         # HarfBuzz 安装指南
```

## 技术细节

### Kerning 提取流程

```
用户启用 Kerning
    ↓
LvglExporter::generateKernTables()
    ↓
尝试 HarfBuzzKerning::extractKerning()
    ├─ 创建 HarfBuzz font
    ├─ 对每个字符对执行 shaping
    ├─ 计算 kerning = advance_with_kern - standard_advance
    └─ 转换为 LVGL 格式 (1/16 像素)
    ↓ 成功 → 生成 kerning 表
    ↓ 失败
回退到 OpenTypeKerning::extractKerning()
    └─ 使用 FT_Get_Kerning() (仅支持 kern 表)
    ↓ 成功 → 生成 kerning 表
    ↓ 失败
输出 "No kerning data"
```

### 预览 Kerning 流程

```
FontPreviewWidget::renderPreview()
    ↓
FreeTypeRenderer::getKerning()
    ├─ 检查缓存
    ├─ 尝试 HarfBuzz shaping
    └─ 回退到 FT_Get_Kerning()
    ↓
应用 kerning 调整字符位置
    ↓
渲染预览图像
```

## 已知问题

### 性能

- 对于大字符集（1000+ 字符），kerning 提取可能需要几秒钟
- 这是正常的，因为需要测试 N×N 个字符对
- 使用缓存机制优化预览性能

### 兼容性

- Windows: 需要 MSVC 2022 或 MinGW
- Linux: 需要 GCC 7+ 或 Clang 5+
- macOS: 需要 Xcode 10+

## 故障排除

### 问题：预览没有显示 kerning 效果

**检查：**
1. 是否勾选了 "Enable Kerning"
2. 字体是否包含 kerning 数据
3. 是否使用了新版本（查看编译时间）

**解决：**
- 重新编译：运行 `build_and_deploy.bat`
- 测试已知有 kerning 的字体（如 Arial）

### 问题：生成的 .c 文件显示 "No kerning data"

**检查：**
1. 控制台输出是否显示 "Successfully extracted kerning using HarfBuzz"
2. 是否勾选了 "Enable Kerning"
3. 字符列表中是否包含有 kerning 的字符对（如 AV, To）

**解决：**
- 确保使用新编译的版本
- 查看控制台输出诊断问题

### 问题：程序无法启动

**Windows:**
- 运行 `build_and_deploy.bat` 重新部署依赖
- 检查是否缺少 DLL

**Linux/macOS:**
- 检查是否安装了所有依赖库
- 运行 `ldd build/LvglFontGenerator` 查看缺失的库

## 贡献

欢迎提交 Issue 和 Pull Request！

### 开发环境设置

1. 克隆仓库
2. 安装依赖（参考 BUILD_DEPLOY_GUIDE.md）
3. 运行编译脚本
4. 使用 Qt Creator 进行开发

### 代码风格

- 使用 Qt 代码风格
- 函数名使用 camelCase
- 类名使用 PascalCase
- 添加必要的注释

## 许可证

[根据原项目许可证]

## 致谢

- **LVGL** - 轻量级图形库
- **HarfBuzz** - OpenType 文本 shaping 引擎
- **FreeType** - 字体渲染库
- **Qt** - 跨平台应用框架

## 更新日志

### v2.0.0 (2026-06-01)

**重大更新：集成 HarfBuzz**

- ✅ 添加 HarfBuzz 支持，完全支持 GPOS kerning
- ✅ 修复预览窗口 kerning 显示
- ✅ 添加 kerning 缓存优化性能
- ✅ 添加自动编译部署脚本
- ✅ 完善文档和使用指南

**测试：**
- ✅ Arial Regular/Bold - 成功提取 kerning
- ✅ MyriadPro Bold - 成功提取 kerning
- ✅ 预览正确显示 kerning 效果
- ✅ 导出文件包含完整 kerning 表

---

**现在 LvglFontGenerator 可以完美处理所有现代字体的 kerning 数据了！** 🎉
