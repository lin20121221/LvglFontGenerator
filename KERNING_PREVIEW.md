# Kerning预览功能实现

## 功能说明

实现了字体预览窗口对Kerning开关的实时响应，用户可以直观地看到启用/禁用Kerning的效果差异。

## 实现内容

### 1. FontPreviewWidget增强

**新增方法：**
```cpp
void setFontPath(const QString &fontPath, int fontSize);
void setEnableKerning(bool enable);
```

**新增成员变量：**
```cpp
QString m_fontPath;      // 字体文件路径
int m_fontSize;          // 字体大小
bool m_enableKerning;    // 是否启用kerning
```

**新增私有方法：**
```cpp
void renderPreviewWithQt();       // 使用Qt渲染（无kerning）
void renderPreviewWithKerning();  // 使用FreeType渲染（有kerning）
```

### 2. 渲染逻辑

**renderPreview()** - 主渲染入口
- 检查是否启用kerning且有字体路径
- 如果是，调用`renderPreviewWithKerning()`
- 否则，调用`renderPreviewWithQt()`

**renderPreviewWithQt()** - Qt渲染（原有逻辑）
- 使用Qt的字体渲染系统
- 不支持kerning控制
- 快速但无法显示kerning效果

**renderPreviewWithKerning()** - FreeType渲染（新增）
- 使用FreeType逐字符渲染
- 计算并应用kerning值
- 精确显示kerning效果

### 3. Kerning应用逻辑

```cpp
// 遍历每个字符
for (int i = 0; i < m_previewText.length(); i++) {
    // 渲染当前字符
    renderGlyph(charCode, glyph);
    
    // 移动到下一个位置
    xPos += glyph.advance_x;
    
    // 应用kerning（如果不是最后一个字符）
    if (i < m_previewText.length() - 1) {
        int kernValue = renderer.getKerning(charCode, nextCharCode);
        xPos += kernValue / 16.0;  // 转换为像素
    }
}
```

### 4. MainWindow集成

**连接信号：**
```cpp
connect(ui->checkEnableKerning, &QCheckBox::toggled,
        this, &MainWindow::onKerningToggled);
```

**槽函数实现：**
```cpp
void MainWindow::onKerningToggled(bool checked)
{
    previewWidget->setEnableKerning(checked);
}
```

**更新预览：**
```cpp
void MainWindow::updatePreview()
{
    previewWidget->setFont(currentFont);
    previewWidget->setFontPath(currentFontPath, ui->spinFontSize->value());
    previewWidget->setEnableKerning(ui->checkEnableKerning->isChecked());
    // ...
}
```

## 使用方法

1. **选择字体文件**
2. **输入预览文本**（建议使用包含明显kerning对的文本）
3. **勾选/取消"启用Kerning"复选框**
4. **预览窗口实时更新**，显示kerning效果

## 推荐测试文本

为了更明显地看到kerning效果，建议使用以下文本：

- **英文大写**：`AVATAR WAVE TALL`
- **混合大小写**：`Typography Quality`
- **特定字符对**：`AV To Wa LT Yo`

这些文本包含了kerning调整较大的字符对。

## 效果对比

### 禁用Kerning（使用Qt渲染）
- 字符间距固定
- 某些字符对显得松散或拥挤
- 渲染速度快

### 启用Kerning（使用FreeType渲染）
- 字符间距根据字符对优化
- 视觉效果更专业
- 渲染速度略慢（但预览可接受）

## 技术细节

### 坐标系统

**FreeType坐标系：**
- 原点在基线左侧
- Y轴向上为正
- `bitmap_top`：字形顶部到基线的距离

**QImage坐标系：**
- 原点在左上角
- Y轴向下为正

**转换公式：**
```cpp
int glyphX = xPos + glyph.bitmap_left;
int glyphY = baseline - glyph.bitmap_top;
```

### 像素值反转

FreeType和QImage的像素值含义相反：
- **FreeType**: 0=背景（白），255=前景（黑）
- **QImage**: 0=黑色，255=白色

**转换：**
```cpp
tempImage.setPixel(imgX, imgY, 255 - pixelValue);
```

### Kerning值单位

- **FreeType返回**: 1/16像素为单位
- **显示需要**: 像素为单位
- **转换**: `kernValue / 16.0`

## 修改的文件

1. **src/fontpreviewwidget.h**
   - 添加`setFontPath()`方法
   - 添加`setEnableKerning()`方法
   - 添加成员变量
   - 声明新的私有渲染方法

2. **src/fontpreviewwidget.cpp**
   - 包含`freetyperenderer.h`
   - 实现`setFontPath()`和`setEnableKerning()`
   - 重构`renderPreview()`
   - 实现`renderPreviewWithQt()`
   - 实现`renderPreviewWithKerning()`

3. **src/mainwindow.h**
   - 添加`onKerningToggled()`槽函数声明

4. **src/mainwindow.cpp**
   - 连接kerning复选框信号
   - 实现`onKerningToggled()`槽函数
   - 更新`updatePreview()`传递字体路径和kerning状态

## 性能考虑

### 预览渲染性能
- **Qt渲染**：非常快（毫秒级）
- **FreeType渲染**：稍慢（几十毫秒）
- **影响**：预览窗口更新时有轻微延迟，但完全可接受

### 优化建议
- 只在需要时使用FreeType渲染（启用kerning时）
- 缓存渲染结果，避免重复渲染
- 限制预览文本长度

## 用户体验

### 实时反馈
用户勾选/取消"启用Kerning"复选框时，预览窗口立即更新，可以直观对比效果。

### 视觉对比
通过快速切换kerning开关，用户可以清楚地看到：
- 字符间距的变化
- 排版质量的提升
- 是否值得启用kerning

## 已知限制

1. **系统字体**：如果选择系统字体（而非文件），kerning预览可能不可用
2. **预览文本长度**：过长的文本可能导致渲染变慢
3. **字体支持**：只有包含kerning信息的字体才会显示效果

## 总结

Kerning预览功能让用户可以：
- ✅ 实时看到kerning效果
- ✅ 对比启用/禁用的差异
- ✅ 决定是否需要启用kerning
- ✅ 选择合适的测试文本

这大大提升了工具的易用性和专业性。
