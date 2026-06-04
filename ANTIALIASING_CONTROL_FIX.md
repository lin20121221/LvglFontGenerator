# 预览抗锯齿控制修复

## 问题描述

用户反馈：预览缩略图中不要做抗锯齿。查看截图发现即使 BPP 设置为 1，字符网格缩略图仍然显示抗锯齿效果。

## 根本原因

FreeType 渲染时始终使用 `FT_LOAD_TARGET_LIGHT`（抗锯齿模式），导致所有预览都是平滑的灰度图像，即使 BPP 设置为 1 也显示抗锯齿效果。

**影响范围**：
1. ✅ 字符预览窗口（已修复）
2. ❌ 字符网格缩略图（需要修复）← 用户截图中的问题

## 解决方案

**根据 BPP 设置动态选择渲染模式**：
- **BPP 1**：使用单色渲染（无抗锯齿）
- **BPP 2/4/8**：使用抗锯齿渲染

这样预览就能准确反映最终输出的效果。

## 代码变更

### 1. 添加抗锯齿控制接口

**文件**: `src/freetyperenderer.h`

```cpp
class FreeTypeRenderer
{
public:
    // 设置渲染模式：是否使用抗锯齿
    void setAntialiasing(bool enabled) { m_antialiasing = enabled; }

private:
    bool m_antialiasing;    // 是否使用抗锯齿渲染
};
```

### 2. 实现抗锯齿控制

**文件**: `src/freetyperenderer.cpp`

#### 构造函数
```cpp
FreeTypeRenderer::FreeTypeRenderer()
    : m_library(nullptr)
    , m_face(nullptr)
    , m_hb_font(nullptr)
    , m_initialized(false)
    , m_antialiasing(true)  // 默认启用抗锯齿
{
}
```

#### 渲染时选择模式
```cpp
bool FreeTypeRenderer::renderGlyph(uint32_t charCode, GlyphData &outGlyph)
{
    // ...
    
    // 根据抗锯齿设置选择渲染模式
    FT_Int32 load_flags;
    if (m_antialiasing) {
        // 使用抗锯齿（与官方工具一致）
        load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT;
    } else {
        // 单色渲染（无抗锯齿，适合 BPP 1）
        load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME;
    }

    FT_Error error = FT_Load_Glyph(m_face, glyph_index, load_flags);
    // ...
}
```

### 3. 字符预览窗口设置抗锯齿

**文件**: `src/fontpreviewwidget.cpp`

```cpp
void FontPreviewWidget::renderPreviewWithFreeType()
{
    // ...
    
    FreeTypeRenderer renderer;
    if (!renderer.loadFont(m_fontPath, m_fontSize)) {
        return;
    }

    // 根据 BPP 设置抗锯齿：BPP 1 不使用抗锯齿，其他使用
    renderer.setAntialiasing(m_bpp > 1);
    
    // ... 继续渲染 ...
}
```

### 4. 字符网格缩略图设置抗锯齿 ⭐ 新增

**文件**: `src/charactergridwidget.cpp`

```cpp
void CharacterGridWidget::renderThumbnailsAsync()
{
    // ...
    
    // 复制必要的数据到 lambda
    QString fontPath = m_fontPath;
    int fontSize = m_fontSize;
    int bpp = m_bpp;  // 添加 BPP
    QVector<CharacterGridItem> items = m_items;

    // 在后台线程渲染
    QFuture<QVector<CharacterGridItem>> future = QtConcurrent::run([this, fontPath, fontSize, bpp, items]() {
        QVector<CharacterGridItem> renderedItems = items;

        FreeTypeRenderer renderer;
        if (!renderer.loadFont(fontPath, fontSize)) {
            return renderedItems;
        }

        // 根据 BPP 设置抗锯齿：BPP 1 不使用抗锯齿，其他使用
        renderer.setAntialiasing(bpp > 1);

        // 渲染每个字符...
    });
}
```

**关键点**：
1. 在 lambda 函数中捕获 `bpp` 参数
2. 在渲染器加载字体后立即设置抗锯齿模式
3. 确保与字符预览窗口使用相同的逻辑

## FreeType 渲染模式对比

### 抗锯齿模式（BPP 2/4/8）

```cpp
FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT
```

**效果**：
- 平滑的灰度边缘
- 256 级灰度
- 适合 BPP 2/4/8

### 单色模式（BPP 1）

```cpp
FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME
```

**效果**：
- 纯黑白像素
- 无灰度过渡
- 适合 BPP 1

## 行为说明

| BPP 设置 | 渲染模式 | 预览效果 | 网格缩略图 |
|---------|---------|---------|-----------|
| **1** | 单色（MONO） | 纯黑白，清晰边缘 | 纯黑白 ✅ |
| **2** | 抗锯齿（LIGHT） | 4 级灰度平滑 | 平滑 ✅ |
| **4** | 抗锯齿（LIGHT） | 16 级灰度平滑 | 平滑 ✅ |
| **8** | 抗锯齿（LIGHT） | 256 级灰度平滑 | 平滑 ✅ |

## 用户体验

### 之前（修复前）

- 所有 BPP 设置都显示抗锯齿预览
- BPP 1 预览有灰度，但实际输出是纯黑白
- 预览与实际输出不一致
- **字符网格缩略图也是抗锯齿**（用户截图）

### 现在（修复后）

- BPP 1：预览和缩略图都显示纯黑白（与输出一致）✅
- BPP 2/4/8：预览和缩略图都显示抗锯齿（高质量）✅
- 预览准确反映渲染模式
- **字符网格缩略图也正确显示单色**✅

## 验证

启动 LvglFontGenerator：

1. 设置 **BPP = 1**
   - 字符预览窗口应该显示**纯黑白像素**
   - **字符网格缩略图也应该显示纯黑白**✅
2. 设置 **BPP = 4**
   - 字符预览窗口应该显示**平滑灰度**
   - **字符网格缩略图也应该显示平滑灰度**✅
3. 切换 BPP 时
   - 预览实时更新渲染模式
   - **网格缩略图也会重新渲染**✅

## 修复日期

2026-06-05

## 状态

✅ **完成** - 预览和字符网格缩略图现在都根据 BPP 设置动态选择是否使用抗锯齿，BPP 1 显示纯黑白像素。
