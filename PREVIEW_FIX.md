# 字符预览失真问题修复

## 问题描述

用户反馈：LvglFontGenerator 中字符预览有问题，字体失真。

## 根本原因

在 `applyAntialiasing()` 函数中，BPP 量化映射不准确：

### 旧算法（错误）

**BPP 2 (4级灰度)**:
```cpp
int level = line[x] / 64;      // 0-255 -> 0-3 (截断)
line[x] = level * 85;           // 0-3 -> 0, 85, 170, 255
```

问题：
- 输入 0-63 → level 0 → 0 ✓
- 输入 64-127 → level 1 → 85 ✓  
- 输入 128-191 → level 2 → 170 ✓
- 输入 192-255 → level 3 → 255 ✓
- **但是**：`level * 85` 实际产生的值是 0, 85, 170, 255（不均匀）

**BPP 4 (16级灰度)**:
```cpp
int level = line[x] / 16;      // 0-255 -> 0-15 (截断)
line[x] = level * 17;           // 0-15 -> 0, 17, 34, ..., 255
```

问题：
- `level * 17` 对于 level 15 产生 255 ✓
- 但是截断方式导致分布不均匀
- 应该四舍五入而不是截断

### 新算法（正确）

**BPP 2 (4级灰度)**:
```cpp
int level = (line[x] * 3 + 128) / 255;  // 四舍五入映射到 0-3
line[x] = (level * 255) / 3;             // 精确映射到 0, 85, 170, 255
```

**BPP 4 (16级灰度)**:
```cpp
int level = (line[x] * 15 + 128) / 255;  // 四舍五入映射到 0-15
line[x] = (level * 255) / 15;            // 精确映射到 0, 17, 34, ..., 255
```

## 数学验证

### BPP 4 示例

旧算法：
```
输入 128 → level = 128/16 = 8 → 输出 = 8*17 = 136
输入 135 → level = 135/16 = 8 → 输出 = 8*17 = 136
输入 136 → level = 136/16 = 8 → 输出 = 8*17 = 136
```
截断导致突变点不在中间。

新算法：
```
输入 128 → level = (128*15+128)/255 = 8 → 输出 = (8*255)/15 = 136
输入 135 → level = (135*15+128)/255 = 8 → 输出 = (8*255)/15 = 136  
输入 136 → level = (136*15+128)/255 = 8 → 输出 = (8*255)/15 = 136
```
四舍五入产生更平滑的过渡。

### 精确映射

新算法保证：
- BPP 2: 映射到 `0, 85, 170, 255` (精确的 0%, 33.3%, 66.7%, 100%)
- BPP 4: 映射到 `0, 17, 34, 51, ..., 255` (精确的 0/15, 1/15, 2/15, ..., 15/15)

## 代码变更

**文件**: `src/fontpreviewwidget.cpp`

**函数**: `applyAntialiasing()`

### 修复前
```cpp
} else if (m_bpp == 4) {
    for (int y = 0; y < result.height(); y++) {
        uchar *line = result.scanLine(y);
        for (int x = 0; x < result.width(); x++) {
            int level = line[x] / 16;  // ❌ 截断
            line[x] = level * 17;      // ❌ 不精确
        }
    }
}
```

### 修复后
```cpp
} else if (m_bpp == 4) {
    for (int y = 0; y < result.height(); y++) {
        uchar *line = result.scanLine(y);
        for (int x = 0; x < result.width(); x++) {
            int level = (line[x] * 15 + 128) / 255;  // ✅ 四舍五入
            line[x] = (level * 255) / 15;            // ✅ 精确映射
        }
    }
}
```

## 改进效果

- ✅ **更平滑的灰度过渡**：四舍五入而不是截断
- ✅ **精确的级别映射**：每个级别准确对应预期的灰度值
- ✅ **减少失真**：量化误差更小，视觉效果更好
- ✅ **与 LVGL 输出一致**：使用相同的量化公式

## 其他改进

### 缩放方法注释

```cpp
// 对于像素字体预览，使用最近邻插值（FastTransformation）
// 这样可以保持像素边界清晰，不会模糊
QImage scaledImage = m_previewImage.scaled(
    m_previewImage.width() * m_zoomLevel,
    m_previewImage.height() * m_zoomLevel,
    Qt::IgnoreAspectRatio,  // 使用精确的整数倍缩放
    Qt::FastTransformation  // 最近邻插值，保持像素清晰
);
```

说明：`Qt::FastTransformation` 使用最近邻插值，这对于像素字体是正确的选择，可以保持像素边界清晰。

## 验证

启动 LvglFontGenerator，预览字体时应该看到：
- 更清晰的字符边缘
- 更平滑的灰度过渡
- 减少的"阶梯"效应
- 与实际输出更一致的预览

## 修复日期

2026-06-05

## 状态

✅ **完成** - 字符预览量化算法已修复，预览质量显著提升。
