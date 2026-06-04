# 字体 Kerning 数据检查报告

## 测试日期
2026-06-01

## 测试字体

### 1. arial.ttf (Arial Regular)
- **kern 表**: ✓ 包含 909 个 kerning 对
- **GPOS 表**: ✓ 包含 10 个 kern 功能 (LookupType 9 - ExtensionPos)
- **FreeType 提取**: ✗ 失败（返回 0）
- **HarfBuzz 提取**: ✓ 成功

**测试结果**:
```
A-V: -19 (1/16 px)
T-o: -19 (1/16 px)
V-A: -19 (1/16 px)
```

---

### 2. arialbd.ttf (Arial Bold)
- **kern 表**: ✓ 包含 908 个 kerning 对
- **GPOS 表**: ✓ 包含 10 个 kern 功能 (LookupType 9 - ExtensionPos)
- **FreeType 提取**: ✗ 失败（返回 0）
- **HarfBuzz 提取**: ✓ 成功

**测试结果**:
```
A-V: -19 (1/16 px)
T-o: -19 (1/16 px)
V-A: -19 (1/16 px)
W-A: -14 (1/16 px)
Y-o: -19 (1/16 px)
F-,: -28 (1/16 px)
```

---

### 3. MyriadPro-Bold-Revised_20250304.ttf
- **kern 表**: ✗ 不包含
- **GPOS 表**: ✓ 包含 1 个 kern 功能 (LookupType 2 - PairPos)
- **FreeType 提取**: ⚠ 未知（可能失败）
- **HarfBuzz 提取**: ✓ 成功

**测试结果**:
```
A-V: -15 (1/16 px)
T-o: -21 (1/16 px)
V-A: -14 (1/16 px)
W-A: -14 (1/16 px)
Y-o: -28 (1/16 px)
F-,: -23 (1/16 px)
T-a: -17 (1/16 px)
P-.: -37 (1/16 px)
```

---

## 问题分析

### FreeType `FT_Get_Kerning()` 的限制

1. **对于 Arial 字体**:
   - 虽然同时包含 kern 表和 GPOS 表
   - FreeType 优先使用 GPOS 表
   - GPOS 使用 LookupType 9 (ExtensionPos) 格式
   - `FT_Get_Kerning()` 无法处理这种复杂格式
   - 也没有正确回退到 kern 表

2. **对于 MyriadPro 字体**:
   - 只包含 GPOS 表（无 kern 表）
   - GPOS 使用 LookupType 2 (PairPos) 格式
   - `FT_Get_Kerning()` 可能支持，但需要测试确认

### HarfBuzz 的优势

- ✓ 完全支持所有 GPOS 表格式
- ✓ 支持 LookupType 2 (PairPos)
- ✓ 支持 LookupType 9 (ExtensionPos)
- ✓ 可以处理复杂的 OpenType 特性
- ✓ 是现代文本渲染的行业标准

---

## 结论

### 所有测试字体都包含 kerning 数据

| 字体 | kern 表 | GPOS 表 | FreeType | HarfBuzz |
|------|---------|---------|----------|----------|
| arial.ttf | ✓ (909对) | ✓ (Type 9) | ✗ | ✓ |
| arialbd.ttf | ✓ (908对) | ✓ (Type 9) | ✗ | ✓ |
| MyriadPro-Bold | ✗ | ✓ (Type 2) | ⚠ | ✓ |

### 推荐方案

**使用 HarfBuzz 库**替代 FreeType 的 `FT_Get_Kerning()` 函数：

1. ✓ 可以提取所有字体的 kerning 数据
2. ✓ 支持所有 OpenType 格式
3. ✓ 向后兼容（保留 FreeType 作为回退）
4. ✓ 未来可扩展支持更多排版特性

---

## 实施状态

✓ HarfBuzz 集成已完成
✓ 编译成功
✓ 测试验证通过

LvglFontGenerator 现在可以正确识别和导出所有字体的 kerning 数据！
