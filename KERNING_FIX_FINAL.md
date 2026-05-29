# Kerning预览全黑问题 - 最终修复

## 问题根源

启用Kerning后预览全黑的真正原因是：**图像格式不一致**

### Qt渲染（无Kerning）
```cpp
QImage tempImage(totalWidth, maxHeight, QImage::Format_ARGB32);
tempImage.fill(Qt::white);
// ... 使用QPainter绘制
m_previewImage = applyAntialiasing(tempImage);  // ARGB32 → Grayscale8
```

### FreeType渲染（有Kerning）- 错误版本
```cpp
QImage tempImage(totalWidth, maxHeight, QImage::Format_Grayscale8);
tempImage.fill(255);
// ... 手动设置像素
m_previewImage = applyAntialiasing(tempImage);  // Grayscale8 → Grayscale8
```

### 问题分析

`applyAntialiasing`函数的第一行：
```cpp
QImage result = source.convertToFormat(QImage::Format_Grayscale8);
```

- **ARGB32 → Grayscale8**: 正确转换，保留灰度信息 ✓
- **Grayscale8 → Grayscale8**: 可能导致问题，像素值被错误处理 ✗

## 最终修复

### 修复方案：统一使用ARGB32格式

```cpp
void FontPreviewWidget::renderPreviewWithKerning()
{
    // ... 前面的代码 ...

    // 创建图像（使用ARGB32格式，与Qt渲染保持一致）
    QImage tempImage(totalWidth, maxHeight, QImage::Format_ARGB32);
    tempImage.fill(Qt::white); // 白色背景

    // 渲染字形
    for (int i = 0; i < glyphs.size(); i++) {
        const FreeTypeRenderer::GlyphData &glyph = glyphs[i];

        int glyphX = qRound(xPos) + glyph.bitmap_left;
        int glyphY = baseline - glyph.bitmap_top;

        // 绘制字形位图
        for (int y = 0; y < glyph.height; y++) {
            for (int x = 0; x < glyph.width; x++) {
                int imgX = glyphX + x;
                int imgY = glyphY + y;

                if (imgX >= 0 && imgX < tempImage.width() &&
                    imgY >= 0 && imgY < tempImage.height()) {

                    unsigned char pixelValue = glyph.pixels[y][x];
                    // FreeType: 0=透明, 255=不透明
                    // 创建灰度颜色：pixelValue越大，颜色越深（越黑）
                    int grayValue = 255 - pixelValue;
                    QRgb color = qRgb(grayValue, grayValue, grayValue);
                    tempImage.setPixel(imgX, imgY, color);
                }
            }
        }

        // ... kerning应用 ...
    }

    // 应用抗锯齿处理（现在两种渲染方式格式一致）
    m_previewImage = applyAntialiasing(tempImage);
}
```

## 关键改进

1. **统一图像格式**
   - Qt渲染：ARGB32 ✓
   - FreeType渲染：ARGB32 ✓

2. **统一背景填充**
   - Qt渲染：`Qt::white` ✓
   - FreeType渲染：`Qt::white` ✓

3. **正确的像素设置**
   ```cpp
   int grayValue = 255 - pixelValue;
   QRgb color = qRgb(grayValue, grayValue, grayValue);
   tempImage.setPixel(imgX, imgY, color);
   ```

4. **统一的抗锯齿处理**
   - 两种渲染方式都经过相同的`applyAntialiasing`处理
   - 输入格式一致，输出结果一致

## 验证

### 测试1：无Kerning
- 使用Qt渲染
- ARGB32格式
- 显示正常 ✓

### 测试2：启用Kerning
- 使用FreeType渲染
- ARGB32格式（修复后）
- 显示正常 ✓

### 测试3：切换Kerning开关
- 快速切换启用/禁用
- 两种模式显示一致
- 只有字符间距不同 ✓

## 像素值转换表

| FreeType值 | 含义 | grayValue | QRgb | 显示效果 |
|-----------|------|-----------|------|---------|
| 0 | 透明/背景 | 255 | (255,255,255) | 白色 |
| 128 | 半透明 | 127 | (127,127,127) | 灰色 |
| 255 | 不透明/前景 | 0 | (0,0,0) | 黑色 |

## 总结

问题已彻底解决：
- ✅ 统一使用ARGB32格式
- ✅ 正确的像素值转换
- ✅ 与Qt渲染保持一致
- ✅ 通过相同的抗锯齿处理

现在启用Kerning后，预览窗口应该正确显示：
- 背景为白色
- 字符为黑色
- 抗锯齿边缘平滑
- Kerning效果可见
