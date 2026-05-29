# 像素打包顺序修复

## 问题发现

通过全面对比发现，某些字符的位图数据长度不匹配：

```
U+0021 '!' (宽5×高12):
  官方工具: 30字节
  当前工具: 36字节 ❌

U+0022 '"' (宽7×高5):
  官方工具: 18字节
  当前工具: 20字节 ❌

U+0023 '#' (宽10×高12):
  官方工具: 60字节
  当前工具: 60字节 ✓
```

**规律：奇数宽度的字符有问题，偶数宽度的字符正确。**

## 根本原因

### 错误的实现（修复前）

```cpp
// 按行循环，每行独立打包
for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x += 2) {
        // 打包2个像素
        // 如果宽度是奇数，最后一个像素会填充0
    }
}
```

对于宽度5的字符（4bpp）：
- 第1行：5像素 → 3字节（2+2+1+填充）
- 第2行：5像素 → 3字节
- ...
- 第12行：5像素 → 3字节
- **总计：12行 × 3字节 = 36字节** ❌

### 正确的实现（修复后）

```cpp
// 连续打包所有像素，不按行分割
int totalPixels = width * height;
for (int i = 0; i < totalPixels; i += 2) {
    // 打包2个像素
    // 跨行连续，不填充
}
```

对于宽度5的字符（4bpp）：
- 总像素：5 × 12 = 60像素
- 打包：60像素 ÷ 2 = 30字节
- **总计：30字节** ✓

## 官方工具的实现

在 `lib/font/table_glyf.js` 中：

```javascript
storePixelsRaw(bitStream, pixels) {
  for (let y = 0; y < pixels.length; y++) {
    const line = pixels[y];
    for (let x = 0; x < line.length; x++) {
      bitStream.writeBits(line[x], bpp);  // 连续写入位流
    }
  }
}
```

关键点：
- 使用**位流（bitStream）**连续写入
- 不按行对齐（stride=0或no-compress模式）
- 像素跨行连续打包

## 修复方案

修改所有BPP格式的打包逻辑，从按行循环改为按像素索引循环：

### 4bpp修复
```cpp
// 修复前：按行循环
for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x += 2) { ... }
}

// 修复后：连续打包
int pixelIndex = 0;
int totalPixels = width * height;
while (pixelIndex < totalPixels) {
    int y = pixelIndex / width;
    int x = pixelIndex % width;
    // 打包像素
    pixelIndex += 2;
}
```

### 2bpp和1bpp同样修复

## 影响评估

这个修复将解决：
- ✅ 奇数宽度字符的数据长度错误
- ✅ 位图数据完全匹配率从56.4%提升到接近100%

## 验证

修复后重新生成字体并运行：

```bash
python comprehensive_comparison.py
```

预期结果：
- 所有字符的位图长度应该匹配
- 位图匹配率应该从56.4%提升到90%+

## 相关文件

- `src/lvglexporter.cpp` - 修复1/2/4bpp的像素打包顺序
- `comprehensive_comparison.py` - 全面对比工具

## 技术细节

### 为什么之前的实现会产生填充？

按行循环时，每行的像素数可能不是打包单位的整数倍：
- 4bpp：每2个像素打包成1字节
- 宽度5：每行5像素 = 2字节 + 1像素（需要填充）

### 为什么连续打包不需要填充？

连续打包时，一行的最后一个像素可以和下一行的第一个像素打包在一起：
- 第1行最后1个像素 + 第2行第1个像素 = 1字节（无填充）

### Stride参数的作用

官方工具的 `--stride` 参数控制行对齐：
- `--stride 0` 或 `--no-compress`：不对齐，连续打包
- `--stride 1`：每行按1字节对齐
- `--stride 4`：每行按4字节对齐

当前修复实现的是 `--stride 0` 的行为（连续打包，无对齐）。
