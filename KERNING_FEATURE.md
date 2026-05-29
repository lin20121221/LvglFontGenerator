# Kerning功能实现说明

## 功能概述

为LVGL字体生成工具添加了Kerning（字距调整）输出选项，用户可以选择是否在生成的字体文件中包含kerning信息。

## 什么是Kerning？

Kerning（字距调整）是排版中的一个重要概念，用于调整特定字符对之间的间距，使文本看起来更美观、更易读。

### 示例
- **无Kerning**: "AV" 字符间距过大，看起来松散
- **有Kerning**: "AV" 字符间距优化，视觉效果更好

### Kern Classes工作原理
1. 将相似的字符归为一类（例如：A、V、W可能属于同一类）
2. 定义类与类之间的间距调整值
3. 渲染时，根据相邻字符的类别查找对应的调整值

### 优势
- **节省空间**: 不需要为每个字符对都存储调整值
- **效率高**: 通过分类，大幅减少存储的数据量

## 实现的功能

### 1. 配置选项
- 在`FontGenerator::Config`结构中添加了`enableKerning`布尔字段
- 默认值为`false`（关闭）

### 2. UI界面
- 在"生成配置"组中添加了"启用Kerning"复选框
- 支持中英文界面
- 提供了工具提示说明

### 3. 代码生成
当启用Kerning时，生成的C文件将包含：

```c
// 左侧字符类映射
static const uint8_t kern_left_class_mapping[] = { ... };

// 右侧字符类映射
static const uint8_t kern_right_class_mapping[] = { ... };

// 类对之间的间距调整值
static const int8_t kern_class_values[] = { ... };

// Kerning类结构
static const lv_font_fmt_txt_kern_classes_t kern_classes = {
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = N,
    .right_class_cnt     = N
};
```

字体描述符中的相关字段也会相应更新：
```c
.kern_dsc = &kern_classes,  // 指向kerning数据
.kern_scale = 16,           // kerning缩放因子
.kern_classes = 1,          // 启用kern classes
```

### 4. 当前实现状态

**基础框架**: ✓ 已完成
- 配置选项传递
- UI控件
- 代码生成框架

**简化实现**: ✓ 当前版本
- 每个字符分配一个唯一的类
- 所有kerning值设为0（无实际调整）
- 结构完整，可以正常编译和使用

**完整实现**: ⚠ 需要进一步开发
- 需要从字体文件中提取真实的kerning pair信息
- 需要使用FreeType的`FT_Get_Kerning()`函数
- 需要实现字符分类算法

## 字符排序

工具已经确保字符按Unicode编码排序：
- 在`LvglExporter::generateCmapTables()`中对字形排序
- 在`LvglExporter::generateKernTables()`中也对字形排序
- 与官方lv_font_conv工具保持一致

## 使用方法

### 1. 编译项目
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 2. 使用工具
1. 打开LVGL字体生成工具
2. 选择字体文件和配置参数
3. 在"生成配置"中勾选"启用Kerning"（如需要）
4. 点击"生成字体"

### 3. 测试生成的文件
```bash
# 检查单个文件
python test_kerning.py output_font.c

# 比较两个文件（一个启用kerning，一个未启用）
python test_kerning.py font_with_kerning.c font_without_kerning.c
```

## 文件修改清单

### 头文件
- `src/fontgenerator.h`: 添加`enableKerning`配置字段
- `src/lvglexporter.h`: 添加`enableKerning`成员和`generateKernTables()`方法

### 源文件
- `src/fontgenerator.cpp`: 传递`enableKerning`参数
- `src/lvglexporter.cpp`: 实现kerning表生成逻辑
- `src/mainwindow.cpp`: 添加UI控件处理和多语言支持

### UI文件
- `src/mainwindow.ui`: 添加Kerning复选框控件

### 测试文件
- `test_kerning.py`: Kerning功能测试脚本

## 未来改进方向

1. **提取真实Kerning数据**
   - 使用FreeType的`FT_Get_Kerning()`获取字体的kerning pair
   - 实现字符分类算法，优化存储空间

2. **优化分类算法**
   - 根据字符形状特征进行智能分类
   - 减少类的数量，进一步节省空间

3. **支持更多Kerning格式**
   - 支持LVGL的其他kerning格式（如pair格式）
   - 根据字符数量自动选择最优格式

4. **可视化预览**
   - 在预览窗口中显示kerning效果
   - 对比启用/禁用kerning的差异

## 兼容性

- 兼容LVGL 8.x和9.x
- 生成的代码结构与官方lv_font_conv工具一致
- 默认关闭，不影响现有功能

## 注意事项

1. **默认关闭**: Kerning功能默认关闭，需要手动勾选
2. **当前为简化实现**: 生成的kerning值全为0，不会实际调整间距
3. **文件大小**: 启用kerning会增加生成文件的大小
4. **性能影响**: kerning计算会略微增加渲染时间

## 参考资料

- [LVGL字体格式文档](https://docs.lvgl.io/master/overview/font.html)
- [FreeType Kerning文档](https://freetype.org/freetype2/docs/glyphs/glyphs-4.html)
- [官方lv_font_conv工具](https://github.com/lvgl/lv_font_conv)
