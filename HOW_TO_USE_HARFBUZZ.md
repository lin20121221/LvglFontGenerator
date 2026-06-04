# 如何使用带 HarfBuzz 支持的 LvglFontGenerator

## 问题诊断

你生成的 `Z:\My_Font.c` 文件显示：
```
/* No kerning data */
```

这说明：
1. 使用的是**旧版本**的 LvglFontGenerator（没有 HarfBuzz 支持）
2. 或者在生成时**没有勾选 kerning 选项**

## 解决方案

### 步骤 1: 使用新编译的版本

新版本位置：
```
Z:\LvglFontUtility\LvglFontGenerator\build\Release\LvglFontGenerator.exe
```

编译时间：2026-06-01 16:08

### 步骤 2: 运行新版本

**方法 A: 从 Qt Creator 运行**
```bash
cd Z:\LvglFontUtility\LvglFontGenerator
# 用 Qt Creator 打开 CMakeLists.txt
# 点击运行按钮
```

**方法 B: 使用 windeployqt 部署**
```bash
cd Z:\LvglFontUtility\LvglFontGenerator\build\Release
C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe LvglFontGenerator.exe
```

这会自动复制所有需要的 Qt DLL。

**方法 C: 手动复制 DLL**

需要从 `C:\msys64\mingw64\bin` 复制以下 DLL（已完成）：
- ✓ libharfbuzz-0.dll
- ✓ libgraphite2.dll
- ✓ libglib-2.0-0.dll
- ✓ libintl-8.dll
- ✓ libpcre2-8-0.dll

还需要从 `C:\Qt\6.11.0\msvc2022_64\bin` 复制 Qt DLL：
- Qt6Core.dll
- Qt6Gui.dll
- Qt6Widgets.dll
- 以及其他依赖的 DLL

### 步骤 3: 生成字体时启用 Kerning

在 LvglFontGenerator 界面中：
1. 选择字体文件（如 arialbd.ttf 或 MyriadPro-Bold）
2. **勾选 "Enable Kerning" 选项** ✓
3. 设置其他参数（字号、BPP 等）
4. 点击生成

### 步骤 4: 验证输出

生成的 .c 文件应该包含：

```c
/*--------------------
 *  KERNING
 *-------------------*/

/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 1, 2, ...
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 2, ...
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, -19, -21, ...
};
```

而不是：
```c
/* No kerning data */
```

### 步骤 5: 检查调试输出

运行时应该在控制台看到：
```
HarfBuzz kerning extraction:
  Font: Z:/wqy-zenhei/arialbd.ttf
  Size: 16 px
  Units per EM: 2048
  Characters: 67
Successfully extracted kerning using HarfBuzz
Loaded XXX non-zero kerning pairs
```

如果看到 "Successfully extracted kerning using HarfBuzz"，说明工作正常！

## 快速测试

使用以下字体测试（已验证包含 kerning）：
- `Z:\wqy-zenhei\arial.ttf`
- `Z:\wqy-zenhei\arialbd.ttf`
- `Z:\wqy-zenhei\MyriadPro-Bold-Revised_20250304.ttf`

测试字符串（包含常见 kerning 对）：
```
AVTOWAVY
```

## 常见问题

### Q: 为什么我的字体没有 kerning？
A: 确保：
1. 使用的是新编译的版本（2026-06-01 16:08 或更新）
2. 勾选了 "Enable Kerning" 选项
3. 字体文件确实包含 kerning 数据

### Q: 如何确认使用的是新版本？
A: 查看调试输出，应该看到 "HarfBuzz kerning extraction" 而不是 "C++ kerning extraction"

### Q: 旧版本和新版本的区别？
A: 
- **旧版本**: 使用 FreeType `FT_Get_Kerning()` → 对复杂 GPOS 表失败
- **新版本**: 使用 HarfBuzz → 支持所有 GPOS 格式

## 需要帮助？

如果仍然无法生成 kerning 数据，请提供：
1. 使用的字体文件路径
2. LvglFontGenerator 的控制台输出
3. 生成的 .c 文件的 KERNING 部分
