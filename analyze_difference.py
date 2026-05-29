#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
分析两个字体文件的差异
"""

def extract_first_glyph_data(filename):
    """提取第一个字形(U+0021 "!")的位图数据"""
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()

    # 找到 U+0021 的数据
    start_marker = "/* U+0021"
    start_idx = content.find(start_marker)
    if start_idx == -1:
        return None

    # 找到下一个注释或结束
    next_marker_idx = content.find("/* U+", start_idx + len(start_marker))
    if next_marker_idx == -1:
        next_marker_idx = content.find("};", start_idx)

    data_section = content[start_idx:next_marker_idx]

    # 提取十六进制数据
    import re
    hex_values = re.findall(r'0x([0-9a-fA-F]+)', data_section)

    return [int(h, 16) for h in hex_values]

# 读取两个文件
official_data = extract_first_glyph_data(r"Z:\My_Font_1.c")
current_data = extract_first_glyph_data(r"Z:\My_Font.c")

print("Official tool U+0021 data:")
print(f"  Data length: {len(official_data)} bytes")
print(f"  First 20 bytes: {official_data[:20]}")
print(f"  Data range: {min(official_data)} - {max(official_data)}")

print("\nCurrent tool U+0021 data:")
print(f"  Data length: {len(current_data)} bytes")
print(f"  First 20 bytes: {current_data[:20]}")
print(f"  Data range: {min(current_data)} - {max(current_data)}")

print("\nAnalysis:")
print(f"  Official length / Current length = {len(official_data) / len(current_data):.2f}")

# Check 4bpp packing
print("\n4bpp packing verification:")
print("  Official tool uses 4bpp, each byte contains 2 pixels")
print("  Example: 0x1f = 0001 1111 = pixel values 1 and 15")
print("  Example: 0xf6 = 1111 0110 = pixel values 15 and 6")

# 尝试解包官方数据
unpacked_official = []
for byte_val in official_data[:10]:
    high_nibble = (byte_val >> 4) & 0x0F  # 高4位
    low_nibble = byte_val & 0x0F          # 低4位
    unpacked_official.extend([high_nibble, low_nibble])

print(f"\n  Official data first 10 bytes unpacked to 4bpp pixel values:")
print(f"  {unpacked_official}")

print(f"\n  Current tool first 20 bytes (8bpp):")
print(f"  {current_data[:20]}")

print("\nConclusion:")
print("  Official tool: 4bpp format, each byte stores 2 pixels (0-15 per pixel)")
print("  Current tool: 8bpp format, each byte stores 1 pixel (0-255 per pixel)")
print("  Fix needed: Modify bitmap generation in lvglexporter.cpp to convert 8-bit grayscale to 4bpp format")
