# 字体生成工具修复说明

## 问题分析

通过对比官方工具 `lv_font_conv` 生成的字体文件和当前工具生成的字体文件，发现以下主要差异：

### 1. 位图数据格式差异（已修复）

**问题：**
- 官方工具：使用 4bpp（每像素4位）格式，每字节存储2个像素，像素值范围 0-15
- 当前工具：使用 8bpp（每像素8位）格式，每字节存储1个像素，像素值范围 0-255

**影响：**
- 当前工具生成的位图数据大小是官方工具的2倍
- LVGL运行时会按照 `bpp=4` 解析数据，导致显示错误

**修复：**
在 `src/lvglexporter.cpp` 的 `generateBitmapArray()` 函数中：
1. 添加了4bpp格式支持
2. 将8位灰度值（0-255）转换为4位值（0-15）：`pixel = (gray * 15 + 127) / 255`
3. 将相邻两个像素打包到一个字节：`packed = (pixel1 << 4) | pixel2`
4. 更新了 `generateGlyphDescArray()` 中的 `bitmap_index` 计算逻辑

### 2. 渲染差异（部分差异）

**观察：**
即使应用了4bpp打包，生成的位图数据与官方工具仍有细微差异。

**可能原因：**
1. FreeType 渲染参数差异
   - 当前工具使用：`FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT`
   - 官方工具可能使用不同的 hinting 或 rendering 模式

2. 字体度量计算差异
   - `advance_x` 的计算方式
   - `bitmap_left` 和 `bitmap_top` 的处理

3. 像素采样差异
   - 抗锯齿算法
   - 伽马校正

**测试结果：**
```
官方工具 U+0021 第一个字节: 0x1f (像素值 1 和 15)
当前工具 U+0021 第一个字节: 0x2f (像素值 2 和 15)
```

差异很小（1 vs 2），但在某些情况下可能影响显示效果。

## 已应用的修复

### 文件：`src/lvglexporter.cpp`

1. **`generateBitmapArray()` 函数**
   - 添加4bpp格式支持
   - 实现8位到4位的转换
   - 实现像素打包逻辑
   - 保留8bpp格式的兼容性

2. **`generateGlyphDescArray()` 函数**
   - 修复4bpp格式下的 `bitmap_index` 计算
   - 使用 `(totalPixels + 1) / 2` 计算字节数

3. **注释国际化**
   - 将中文注释改为英文，提高代码可读性

## 测试建议

1. **功能测试**
   - 使用修复后的工具生成字体文件
   - 在LVGL项目中加载并显示字体
   - 对比显示效果

2. **数据验证**
   - 运行 `analyze_difference.py` 对比数据格式
   - 验证位图数据大小是否正确（应该是原来的一半）
   - 检查 `glyph_dsc` 中的 `bitmap_index` 是否正确

3. **边界情况**
   - 测试奇数宽度的字形（4bpp打包时需要填充）
   - 测试不同的bpp设置（4, 8）
   - 测试不同字体大小

## 进一步优化建议

1. **渲染参数调优**
   - 研究官方工具的 FreeType 参数设置
   - 尝试不同的 `FT_LOAD_TARGET_*` 选项
   - 调整 hinting 策略

2. **添加更多格式支持**
   - 1bpp（单色）
   - 2bpp（4级灰度）
   - 压缩格式

3. **添加验证工具**
   - 位图数据可视化
   - 与官方工具输出的自动对比
   - 字形度量验证

## 参考资料

- LVGL字体格式文档：https://docs.lvgl.io/master/overview/font.html
- lv_font_conv 官方工具：https://github.com/lvgl/lv_font_conv
- FreeType 文档：https://freetype.org/freetype2/docs/reference/
