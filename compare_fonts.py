#!/usr/bin/env python3
"""
对比两个字库文件的差异
"""

# 官方工具生成的字形描述符
official = {
    'adv_w': 144,
    'box_w': 9,
    'box_h': 12,
    'ofs_x': 0,
    'ofs_y': 0,
    'line_height': 12
}

# 本工具生成的字形描述符
current = {
    'adv_w': 192,
    'box_w': 10,
    'box_h': 15,
    'ofs_x': 1,
    'ofs_y': -15,
    'line_height': 16
}

print("字符 '0' 的对比:")
print("=" * 60)
print(f"{'参数':<15} {'官方工具':<20} {'本工具':<20} {'差异'}")
print("-" * 60)

for key in official.keys():
    off_val = official[key]
    cur_val = current[key]
    diff = cur_val - off_val if isinstance(off_val, int) else 'N/A'
    print(f"{key:<15} {off_val:<20} {cur_val:<20} {diff}")

print("\n分析:")
print("=" * 60)
print("1. adv_w 单位: 官方使用 1/16 像素")
print(f"   官方: 144 / 16 = {144/16} 像素")
print(f"   本工具: 192 / 16 = {192/16} 像素")
print()
print("2. ofs_y 含义: 字形顶部相对于基线的偏移")
print("   - 正值: 字形顶部在基线下方")
print("   - 负值: 字形顶部在基线上方")
print("   - 0: 字形顶部在基线上")
print()
print("   官方 ofs_y=0 意味着: 字形顶部在基线位置")
print("   本工具 ofs_y=-15 意味着: 字形顶部在基线上方15像素")
print()
print("3. 问题根源:")
print("   Qt的boundingRect.top()返回的是相对于绘制原点的偏移")
print("   需要转换为相对于基线的偏移")
print()
print("4. 正确的计算方式:")
print("   LVGL的基线位置应该是固定的（通常是ascent）")
print("   ofs_y = boundingRect.top() - (-ascent)")
print("   或者简化为: ofs_y = boundingRect.top() + ascent")
