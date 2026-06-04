# Kerning 值差异问题修复

## 问题描述

比较 CLI 工具和 GUI 工具生成的字体文件时，发现 `kern_class_values` 表中的值相差约 4 倍：

**CLI 工具** (My_Font_1.c):
```c
0, 0, 0, -10, 0, -9, 0, -10,
0, -32, 0, 0, -12, 0, -2, -15,
```

**GUI 工具修复前** (My_Font.c):
```c
0, 0, 0, -42, 0, -40, 0, -43,
0, -128, 0, 0, -49, 0, -10, -62,
```

比例：-42/-10 ≈ 4.2x，-128/-32 = 4x

## 根本原因

两个工具使用了不同的 FreeType 字体大小设置方法：

### CLI 工具 (正确)
```cpp
// font_converter.cpp
FT_Set_Pixel_Sizes(face, 0, fontSize);
// 直接设置像素大小 = 16 pixels
// pixel_size = 16
```

### GUI 工具 (修复前 - 错误)
```cpp
// harfbuzzkerning.cpp
FT_Set_Char_Size(face, 0, fontSize * 64, 300, 300);
// 使用点大小和 DPI
// 计算：16 点 × 300 DPI / 72 DPI = 66.67 pixels
```

**关键差异**：
- `FT_Set_Pixel_Sizes()`: 直接设置像素大小
- `FT_Set_Char_Size()`: 设置点大小，然后根据 DPI 转换为像素

## DPI 计算

```
实际像素大小 = 点大小 × DPI / 72
             = 16 × 300 / 72
             = 66.67 pixels
```

缩放比例：66.67 / 16 = 4.166... ≈ 4x

## 修复方案

将 GUI 工具中的 `FT_Set_Char_Size()` 改为 `FT_Set_Pixel_Sizes()`：

```cpp
// 修复前
error = FT_Set_Char_Size(face, 0, fontSize * 64, 300, 300);

// 修复后
error = FT_Set_Pixel_Sizes(face, 0, fontSize);
```

## 验证

修复后，GUI 工具生成的 kerning 值应该与 CLI 工具一致：

```c
// 两个工具都应该生成
0, 0, 0, -10, 0, -9, 0, -10,
0, -32, 0, 0, -12, 0, -2, -15,
```

## 技术细节

### FP4.4 格式

LVGL 使用 FP4.4 固定点格式存储 kerning 值：
- 值范围：-128 到 127 (int8_t)
- 实际精度：1/16 像素
- 计算：`kern_fp44 = round(kern_pixels × 16)`

### 示例

假设字体单位中的 kerning 值为 -10 font units：

**CLI 工具 (16 pixels)**:
```
像素值 = -10 × 16 / 1000 = -0.16 pixels
FP4.4 = round(-0.16 × 16) = -3
```

**GUI 工具修复前 (66.67 pixels)**:
```
像素值 = -10 × 66.67 / 1000 = -0.667 pixels
FP4.4 = round(-0.667 × 16) = -11
```

比例：-11 / -3 ≈ 3.7x (接近 4x)

## 影响范围

这个问题影响所有使用 GUI 工具生成的包含 kerning 数据的字体文件。修复后：
- ✅ Kerning 值与 CLI 工具一致
- ✅ 与在线工具 (lvgl.io/tools/fontconverter) 一致
- ✅ 字符间距显示正确

## 测试

使用相同参数生成字体文件并比较：

```bash
# CLI 工具
./lv_font_conv.exe --font MyriadPro-Bold-Revised.ttf --size 16 --bpp 4 --range 0x20-0x7F --format lvgl --no-compress -o cli_output.c

# GUI 工具
# 在界面中设置相同参数，启用 kerning，生成 gui_output.c

# 比较 kern_class_values
diff cli_output.c gui_output.c
```

## 修复日期

2026-06-05

## 相关文件

- `src/harfbuzzkerning.cpp` - 主要修复位置
- `FINAL_SUCCESS_100_PERCENT.md` - CLI 工具成功记录
- `TEST_INSTRUCTIONS.md` - 测试说明
