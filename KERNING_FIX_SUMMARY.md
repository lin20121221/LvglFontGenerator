# LvglFontGenerator Kerning 问题修复总结

## 问题描述

LvglFontGenerator 无法正确识别字体文件中的 kerning 数据，即使字体文件（如 arial.ttf, arialbd.ttf）包含完整的 kerning 信息。

## 根本原因

1. **FreeType 的 `FT_Get_Kerning()` 函数存在限制**
   - FreeType 优先使用 GPOS 表而不是传统 kern 表
   - Arial 字体的 GPOS kern 使用了 LookupType 9 (ExtensionPos) 格式
   - `FT_Get_Kerning()` 不完全支持复杂的 GPOS kerning 格式
   - 当 GPOS 处理失败时，FreeType 没有正确回退到 kern 表

2. **测试结果**
   - kern 表包含：908-909 个 kerning 对
   - GPOS 表包含：10 个 kern 功能
   - `FT_Get_Kerning()` 返回：0（失败）

## 解决方案

使用 **HarfBuzz 库**替代 FreeType 的 `FT_Get_Kerning()` 函数。

### 为什么选择 HarfBuzz？

- ✓ 完全支持 GPOS 表的所有格式（包括 ExtensionPos）
- ✓ 可以正确处理复杂的 OpenType kerning
- ✓ 是现代文本渲染的行业标准
- ✓ 被广泛使用（Chrome, Firefox, Android, iOS 等）

### 实现细节

1. **添加 HarfBuzz 依赖**
   - 修改 `CMakeLists.txt` 添加 HarfBuzz 查找和链接
   - 支持 pkg-config 和手动查找
   - 对 MSVC 使用 DLL 导入库以避免兼容性问题

2. **创建 HarfBuzz Kerning 提取器**
   - 新文件：`src/harfbuzzkerning.h` 和 `src/harfbuzzkerning.cpp`
   - 使用 `hb_font_get_glyph_kerning_for_direction()` 提取 kerning
   - 返回格式与原有的 `OpenTypeKerning` 兼容

3. **修改 LvglExporter**
   - 优先使用 HarfBuzz 提取器
   - 如果 HarfBuzz 失败，回退到 FreeType 提取器
   - 保持向后兼容性

## 测试结果

使用 HarfBuzz 成功提取 kerning 数据：

```
字体: arialbd.ttf (16px)
提取结果:
  A-V: -19 (1/16 px)
  T-o: -19 (1/16 px)
  V-A: -19 (1/16 px)
  W-A: -14 (1/16 px)
  Y-o: -19 (1/16 px)
  F-,: -28 (1/16 px)
```

## 安装 HarfBuzz

### Windows (MSYS2)
```bash
pacman -S mingw-w64-x86_64-harfbuzz
```

### Windows (vcpkg)
```bash
vcpkg install harfbuzz:x64-windows
cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```

### Linux
```bash
# Ubuntu/Debian
sudo apt-get install libharfbuzz-dev

# Fedora
sudo dnf install harfbuzz-devel
```

### macOS
```bash
brew install harfbuzz
```

## 编译

```bash
cd build
cmake ..
cmake --build . --config Release
```

## 运行时依赖

Windows 用户需要确保以下 DLL 在可执行文件目录或 PATH 中：
- `libharfbuzz-0.dll`
- `libgraphite2.dll`
- `libglib-2.0-0.dll`
- `libintl-8.dll`
- `libpcre2-8-0.dll`

这些 DLL 位于 `C:/msys64/mingw64/bin/` 目录。

## 文件修改清单

### 新增文件
- `src/harfbuzzkerning.h` - HarfBuzz kerning 提取器头文件
- `src/harfbuzzkerning.cpp` - HarfBuzz kerning 提取器实现
- `HARFBUZZ_INSTALL.md` - HarfBuzz 安装指南
- `test_harfbuzz.cpp` - 测试程序

### 修改文件
- `CMakeLists.txt` - 添加 HarfBuzz 依赖查找和链接
- `src/lvglexporter.cpp` - 优先使用 HarfBuzz 提取器

## 优势

1. **完整支持** - 可以提取所有类型的 kerning 数据
2. **向后兼容** - 保留 FreeType 提取器作为回退方案
3. **行业标准** - 使用广泛认可的文本渲染库
4. **未来扩展** - HarfBuzz 还支持其他高级排版特性

## 验证

编译后运行 LvglFontGenerator，启用 kerning 选项，应该能看到调试输出：

```
HarfBuzz kerning extraction:
  Font: Z:/wqy-zenhei/arialbd.ttf
  Size: 16 px
  Units per EM: 2048
  Characters: 67
Successfully extracted kerning using HarfBuzz
Loaded XXX non-zero kerning pairs
```

如果看到 "Successfully extracted kerning using HarfBuzz"，说明修复成功！
