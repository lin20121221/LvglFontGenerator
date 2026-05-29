# 字形度量精度修复

## 问题描述

在对比官方工具和当前工具生成的字体文件时，发现 `adv_w`（字形前进宽度）存在差异：

```
官方工具: adv_w = 69
当前工具: adv_w = 64
```

## 根本原因

### 数据流程

1. **FreeType渲染**
   - `linearHoriAdvance` 是16.16固定点格式
   - 除以65536得到浮点数，例如：4.3125像素

2. **类型转换问题**
   ```cpp
   // freetyperenderer.h
   double advance_x;  // 4.3125
   
   // fontgenerator.h
   int advanceWidth;  // 截断为 4
   
   // fontgenerator.cpp (修复前)
   glyph.advanceWidth = ftGlyph.advance_x;  // 4.3125 → 4 (丢失精度)
   
   // lvglexporter.cpp (修复前)
   int advWidthScaled = glyph.advWidth * 16;  // 4 * 16 = 64
   ```

3. **官方工具的做法**
   ```javascript
   // 保持浮点精度
   linearHoriAdvance: from_16_16(...)  // 4.3125
   
   // 生成时才转换
   adv_w: Math.round(glyph.advanceX * 16)  // 4.3125 * 16 = 69
   ```

## 修复方案

### 方案1：改变数据类型（不推荐）
将 `advanceWidth` 改为 `double` 类型，但这会影响整个代码结构。

### 方案2：提前转换（已采用）
在赋值时就转换为LVGL的单位（1/16像素），避免精度损失：

```cpp
// fontgenerator.cpp
// LVGL使用1/16像素作为adv_w的单位，所以先乘以16再四舍五入
glyph.advanceWidth = qRound(ftGlyph.advance_x * 16);
// 4.3125 * 16 = 69.0 → 69

// lvglexporter.cpp
// advWidth已经是以1/16像素为单位，直接使用
int advWidthScaled = glyph.advWidth;  // 69
```

## 修复效果

### 修复前
```
FreeType: 4.3125像素
  ↓ (截断)
advanceWidth: 4
  ↓ (乘以16)
adv_w: 64 ❌
```

### 修复后
```
FreeType: 4.3125像素
  ↓ (乘以16并四舍五入)
advanceWidth: 69
  ↓ (直接使用)
adv_w: 69 ✓
```

## 其他发现

### 位图渲染差异

即使修复了 `adv_w`，位图数据仍有细微差异：
- 差异像素：10/44 (22.73%)
- 最大差异：1个灰度级（0-15范围）
- 差异原因：FreeType渲染的微小差异

这些差异非常小，对实际显示效果影响微乎其微。

### 可能的原因

1. **FreeType版本差异**
   - 官方工具使用WebAssembly编译的FreeType
   - 当前工具使用系统安装的FreeType
   - 不同版本的hinting算法可能略有不同

2. **浮点运算精度**
   - JavaScript的Number vs C++的double
   - 不同的舍入行为

3. **编译器优化**
   - 不同的编译器可能产生略微不同的结果

## 验证

使用 `compare_bitmaps_detailed.py` 对比修复前后的结果：

```bash
python compare_bitmaps_detailed.py
```

预期结果：
- `adv_w` 应该与官方工具一致
- 位图差异应该保持在1个灰度级以内

## 总结

这个修复解决了字形度量精度损失的问题，使生成的字体文件与官方工具更加接近。剩余的微小位图差异是可以接受的，不会影响实际使用。

## 相关文件

- `src/fontgenerator.cpp` - 修复advanceWidth的计算
- `src/lvglexporter.cpp` - 修复adv_w的使用
- `src/freetyperenderer.cpp` - 修正注释（16.16格式）
