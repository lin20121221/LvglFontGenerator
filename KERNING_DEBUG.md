# Kerning预览功能调试指南

## 常见问题和解决方案

### 问题1：启用Kerning后显示错误或空白

**可能原因：**

1. **字体路径为空**
   - 症状：启用kerning后预览窗口变空白
   - 原因：没有选择字体文件，或使用了系统字体
   - 解决：确保选择了字体文件（.ttf/.otf），而不是系统字体

2. **FreeType加载失败**
   - 症状：控制台显示"Failed to load font for kerning preview"
   - 原因：字体文件路径错误或文件损坏
   - 解决：重新选择字体文件

3. **字形渲染失败**
   - 症状：控制台显示"Failed to render glyph for character"
   - 原因：字体不包含某些字符
   - 解决：使用字体支持的字符

4. **图像尺寸无效**
   - 症状：控制台显示"Invalid image size"
   - 原因：计算的图像尺寸为0或负数
   - 解决：检查字体大小设置

## 调试步骤

### 步骤1：检查控制台输出

启用kerning后，查看应用程序的控制台输出，寻找以下信息：

```
Font path is empty, falling back to Qt rendering
Failed to load font for kerning preview: <error message>
Failed to render glyph for character: <char>
No valid glyphs rendered, falling back to Qt rendering
Invalid image size: <width> x <height>
```

### 步骤2：验证字体文件

确保：
- ✅ 使用"浏览..."按钮选择了字体文件
- ✅ 字体文件路径显示在"字体文件"输入框中
- ✅ 字体文件是.ttf或.otf格式
- ❌ 不要使用"系统字体..."按钮（系统字体没有文件路径）

### 步骤3：检查预览文本

确保：
- ✅ 预览文本不为空
- ✅ 预览文本包含字体支持的字符
- ✅ 预览文本长度合理（<100个字符）

### 步骤4：检查字体大小

确保：
- ✅ 字体大小在合理范围内（8-128px）
- ✅ 字体大小不为0

## 代码改进

### 增强的错误处理

已添加以下错误检查：

1. **字体路径检查**
```cpp
if (m_fontPath.isEmpty()) {
    qWarning() << "Font path is empty, falling back to Qt rendering";
    renderPreviewWithQt();
    return;
}
```

2. **字形有效性检查**
```cpp
if (glyphs.isEmpty()) {
    qWarning() << "No valid glyphs rendered, falling back to Qt rendering";
    renderPreviewWithQt();
    return;
}
```

3. **图像尺寸检查**
```cpp
if (totalWidth <= 0 || maxHeight <= 0) {
    qWarning() << "Invalid image size:" << totalWidth << "x" << maxHeight;
    renderPreviewWithQt();
    return;
}
```

4. **坐标四舍五入**
```cpp
int glyphX = qRound(xPos) + glyph.bitmap_left;
```

5. **索引边界检查**
```cpp
if (i < glyphs.size() - 1) {  // 使用glyphs.size()而不是m_previewText.length()
    // 应用kerning
}
```

## 使用建议

### 正确的使用流程

1. **选择字体文件**
   ```
   点击"浏览..." → 选择.ttf或.otf文件 → 确认
   ```

2. **设置字体大小**
   ```
   调整"字体大小"数值（建议18-24px）
   ```

3. **输入预览文本**
   ```
   在"预览文本"框中输入：AVATAR WAVE
   ```

4. **启用Kerning**
   ```
   勾选"启用Kerning"复选框
   ```

5. **观察效果**
   ```
   预览窗口应该显示带kerning的文本
   ```

### 如果仍然出错

**回退到Qt渲染：**
- 取消勾选"启用Kerning"
- 预览窗口会使用Qt渲染（无kerning但稳定）

**检查字体：**
- 尝试使用其他字体文件
- 确认字体文件未损坏
- 使用官方工具测试过的字体（如示例中的MyriadPro）

## 已知限制

1. **系统字体不支持**
   - 使用"系统字体..."选择的字体没有文件路径
   - Kerning预览需要字体文件路径
   - 解决：使用"浏览..."选择字体文件

2. **某些字体格式**
   - 某些特殊格式的字体可能不被FreeType支持
   - 解决：使用标准的.ttf或.otf文件

3. **性能限制**
   - 过长的预览文本可能导致渲染变慢
   - 建议：预览文本长度<50个字符

## 测试用例

### 测试1：基本功能
```
字体：任意.ttf文件
大小：18px
文本：Ag字
Kerning：启用
预期：显示3个字符，间距正常
```

### 测试2：明显Kerning效果
```
字体：MyriadPro-Bold.ttf
大小：24px
文本：AVATAR WAVE
Kerning：启用/禁用切换
预期：可以看到明显的间距差异
```

### 测试3：错误处理
```
字体：使用系统字体
Kerning：启用
预期：自动回退到Qt渲染，显示正常
```

## 调试输出示例

### 正常情况
```
（无错误输出）
预览窗口正常显示文本
```

### 字体路径为空
```
Font path is empty, falling back to Qt rendering
预览窗口使用Qt渲染显示文本
```

### 字体加载失败
```
Failed to load font for kerning preview: Cannot open font file
预览窗口使用Qt渲染显示文本
```

### 字形渲染失败
```
Failed to render glyph for character: �
预览窗口显示其他成功渲染的字符
```

## 总结

Kerning预览功能已添加完善的错误处理：
- ✅ 自动回退到Qt渲染
- ✅ 详细的错误日志
- ✅ 边界检查
- ✅ 用户友好的降级策略

如果遇到问题，请：
1. 检查控制台输出
2. 确认使用字体文件（不是系统字体）
3. 验证预览文本和字体大小
4. 如有需要，取消勾选Kerning使用Qt渲染
