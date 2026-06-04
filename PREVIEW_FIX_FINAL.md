# 字符预览失真问题修复（正确版本）

## 问题描述

用户反馈：LvglFontGenerator 中字符预览有问题，字体失真。对比 LvglFontViewer 的预览是正常的。

## 根本原因

**LvglFontGenerator 对预览图像进行了不必要的二次量化**。

### 架构差异

**LvglFontViewer (查看器)**:
1. 从 LVGL 字体文件读取**已量化**的 BPP 数据（1/2/4/8 位）
2. 使用线性插值将量化值映射到 0-255 显示：
   ```cpp
   int grayValue = 255 - (pixelValue * 255) / maxValue;
   ```
3. 预览显示的是**实际输出的量化效果**

**LvglFontGenerator (生成器) - 修复前**:
1. FreeType 渲染出 **256 级灰度**（高质量）
2. `applyAntialiasing()` 将 256 级灰度**量化**到 BPP 级别（1/2/4/8 位）
3. 预览显示的是**模拟量化后的效果**
4. 问题：**二次量化导致细节丢失和失真**

## 为什么会失真？

预览时不应该模拟量化！

- **FreeType 渲染**：已经是高质量的抗锯齿结果（256 级灰度）
- **预览目的**：显示字体的**渲染质量**，而不是最终输出的量化效果
- **量化时机**：应该在**导出**时进行，而不是预览时

### 对比

**查看器**：显示"这个字体文件导出后是什么样子"（已量化）
**生成器**：显示"这个字体渲染出来是什么样子"（应该是高质量原始渲染）

## 解决方案

**移除预览时的量化处理，直接显示 FreeType 的原始渲染质量。**

### 修复前
```cpp
void FontPreviewWidget::renderPreviewWithFreeType()
{
    // ... FreeType 渲染代码 ...
    
    // ❌ 错误：对预览进行量化处理
    m_previewImage = applyAntialiasing(tempImage);
}

QImage FontPreviewWidget::applyAntialiasing(const QImage &source)
{
    // 将 256 级灰度量化到 BPP 级别
    if (m_bpp == 4) {
        int level = (line[x] * 15 + 128) / 255;
        line[x] = (level * 255) / 15;  // 16 级灰度
    }
    // ...
}
```

问题：256 级 → 量化到 16 级 → **细节丢失**

### 修复后
```cpp
void FontPreviewWidget::renderPreviewWithFreeType()
{
    // ... FreeType 渲染代码 ...
    
    // ✅ 正确：直接显示原始渲染质量
    m_previewImage = tempImage.convertToFormat(QImage::Format_Grayscale8);
}
```

结果：保持 FreeType 的 256 级灰度，**高质量预览**

## 设计理念

### LvglFontGenerator 的预览应该做什么？

**目的**：帮助用户预览字体的**渲染效果**
- ✅ 显示字符形状
- ✅ 显示字体质量
- ✅ 预览 kerning 效果
- ❌ **不应该**模拟 BPP 量化效果

**为什么？**
1. 用户调整字体大小时，需要看到**最佳渲染质量**
2. BPP 是导出参数，不是渲染参数
3. 量化是最后一步（导出时），预览应该显示量化前的高质量结果

### LvglFontViewer 的预览应该做什么？

**目的**：查看**已导出字体文件**的效果
- ✅ 显示实际导出的像素数据
- ✅ 使用线性插值放大显示
- ✅ 反映实际的 BPP 量化效果

## 代码变更

### 1. 移除 FreeType 渲染后的量化

**文件**: `src/fontpreviewwidget.cpp`

**renderPreviewWithFreeType()**:
```cpp
// 修复前
m_previewImage = applyAntialiasing(tempImage);

// 修复后
m_previewImage = tempImage.convertToFormat(QImage::Format_Grayscale8);
```

### 2. 移除 Qt 渲染后的量化

**renderPreviewWithQt()**:
```cpp
// 修复前
m_previewImage = applyAntialiasing(tempImage);

// 修复后
m_previewImage = tempImage.convertToFormat(QImage::Format_Grayscale8);
```

### 3. 保留 applyAntialiasing 函数

虽然当前不使用，但保留以防将来需要模拟量化效果的选项。

## UI 说明

界面上的 **BPP 参数**：
- **当前行为**：仅影响导出的字体文件
- **预览显示**：始终使用 256 级灰度（高质量）
- **用户体验**：看到的是最佳渲染效果，而不是量化后的效果

这是正确的设计，因为：
1. 用户需要看到字体的真实质量
2. BPP 是存储优化，不是渲染参数
3. 预览应该帮助用户选择字体和大小，而不是显示存储格式

## 对比总结

| 工具 | 数据源 | 显示内容 | 是否量化 |
|------|--------|----------|----------|
| **LvglFontViewer** | 字体文件 | 已导出的量化数据 | 是（来自文件）|
| **LvglFontGenerator** | FreeType | 原始渲染质量 | 否（修复后）|

## 验证

启动 LvglFontGenerator，预览字体时应该看到：
- ✅ 清晰的字符边缘
- ✅ 平滑的灰度过渡（256 级）
- ✅ 无阶梯效应
- ✅ 与 FreeType 原始渲染一致
- ✅ 高质量预览

切换 BPP 参数（1/2/4/8）时：
- ✅ 预览**不变**（始终高质量）
- ✅ 导出文件**会改变**（按 BPP 量化）

## 修复日期

2026-06-05

## 状态

✅ **完成** - 移除了不必要的预览量化，现在显示 FreeType 的原始高质量渲染。
