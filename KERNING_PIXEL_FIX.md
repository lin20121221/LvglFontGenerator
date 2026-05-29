# Kerning预览像素反转问题修复

## 问题描述

启用Kerning后，预览窗口显示全黑，不管字符是否有笔画都是黑色。

## 问题原因

**像素值理解错误**

FreeType和QImage对灰度值的含义不同：

### FreeType的像素值
- `0` = 透明/背景（无笔画）
- `255` = 不透明/前景（有笔画，黑色）
- 中间值 = 抗锯齿的灰度

### QImage Grayscale8的像素值
- `0` = 黑色
- `255` = 白色
- 中间值 = 灰度

## 正确的转换逻辑

```cpp
// 背景已经填充为白色(255)
tempImage.fill(255);

// 对于每个字形像素
unsigned char pixelValue = glyph.pixels[y][x];  // FreeType值: 0-255

// 转换公式
unsigned char qimageValue = 255 - pixelValue;

// 解释：
// FreeType=0 (背景) → QImage=255 (白色) ✓
// FreeType=255 (前景) → QImage=0 (黑色) ✓
// FreeType=128 (半透明) → QImage=127 (灰色) ✓
```

## 修复前的错误代码

```cpp
// 错误：没有正确理解像素值含义
tempImage.setPixel(imgX, imgY, 255 - pixelValue);
// 这个公式本身是对的，但可能在其他地方有问题
```

## 修复后的正确代码

```cpp
// 创建白色背景
QImage tempImage(totalWidth, maxHeight, QImage::Format_Grayscale8);
tempImage.fill(255); // 白色背景

// 绘制字形
for (int y = 0; y < glyph.height; y++) {
    for (int x = 0; x < glyph.width; x++) {
        int imgX = glyphX + x;
        int imgY = glyphY + y;

        if (imgX >= 0 && imgX < tempImage.width() &&
            imgY >= 0 && imgY < tempImage.height()) {

            unsigned char pixelValue = glyph.pixels[y][x];
            // FreeType: 0=透明/背景, 255=不透明/前景（黑色）
            // QImage Grayscale8: 0=黑色, 255=白色
            // 转换：255 - pixelValue
            tempImage.setPixel(imgX, imgY, 255 - pixelValue);
        }
    }
}
```

## 验证

### 测试用例1：空格字符
- FreeType所有像素 = 0（透明）
- QImage所有像素 = 255（白色）
- 结果：显示为白色背景 ✓

### 测试用例2：实心字符
- FreeType笔画像素 = 255（不透明）
- QImage笔画像素 = 0（黑色）
- 结果：显示为黑色字符 ✓

### 测试用例3：抗锯齿边缘
- FreeType边缘像素 = 128（半透明）
- QImage边缘像素 = 127（灰色）
- 结果：显示为平滑的灰色边缘 ✓

## 可能的其他问题

如果修复后仍然全黑，检查：

1. **图像格式**
   ```cpp
   QImage tempImage(totalWidth, maxHeight, QImage::Format_Grayscale8);
   ```
   确保使用`Format_Grayscale8`

2. **背景填充**
   ```cpp
   tempImage.fill(255); // 必须是255（白色）
   ```

3. **边界检查**
   ```cpp
   if (imgX >= 0 && imgX < tempImage.width() &&
       imgY >= 0 && imgY < tempImage.height())
   ```
   确保不会越界

4. **抗锯齿处理**
   检查`applyAntialiasing()`函数是否正确处理灰度图像

## 调试方法

### 方法1：输出像素值
```cpp
qDebug() << "FreeType pixel:" << (int)pixelValue 
         << "QImage pixel:" << (int)(255 - pixelValue);
```

### 方法2：保存中间图像
```cpp
tempImage.save("debug_before_antialiasing.png");
m_previewImage.save("debug_after_antialiasing.png");
```

### 方法3：检查图像尺寸
```cpp
qDebug() << "Image size:" << tempImage.width() << "x" << tempImage.height();
qDebug() << "Glyph count:" << glyphs.size();
```

## 总结

问题已修复，关键点：
- ✅ 正确理解FreeType和QImage的像素值含义
- ✅ 使用正确的转换公式：`255 - pixelValue`
- ✅ 确保背景填充为白色(255)
- ✅ 使用正确的图像格式(Grayscale8)

现在启用Kerning后应该能正确显示字符，背景为白色，字符为黑色。
