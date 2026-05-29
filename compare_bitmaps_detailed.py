#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
详细对比两个字体文件的位图数据
"""

import re

def extract_glyph_bitmap(filename, unicode_char):
    """提取指定字符的完整位图数据"""
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()

    # 找到指定字符的数据
    start_marker = f"/* U+{unicode_char:04X}"
    start_idx = content.find(start_marker)
    if start_idx == -1:
        return None, None

    # 找到下一个注释或结束
    next_marker_idx = content.find("/* U+", start_idx + len(start_marker))
    if next_marker_idx == -1:
        next_marker_idx = content.find("};", start_idx)

    data_section = content[start_idx:next_marker_idx]

    # 提取十六进制数据
    hex_values = re.findall(r'0x([0-9a-fA-F]+)', data_section)
    bytes_data = [int(h, 16) for h in hex_values]

    # 提取glyph描述符
    glyph_pattern = r'\{\.bitmap_index = (\d+), \.adv_w = (\d+), \.box_w = (\d+), \.box_h = (\d+), \.ofs_x = (-?\d+), \.ofs_y = (-?\d+)\}'

    # 在glyph_dsc数组中查找
    glyph_dsc_start = content.find('static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[]')
    if glyph_dsc_start == -1:
        return bytes_data, None

    # 计算这是第几个字符（从U+0021开始）
    char_index = unicode_char - 0x0021 + 1  # +1因为id=0是保留的

    # 查找对应的描述符
    glyph_dsc_section = content[glyph_dsc_start:glyph_dsc_start + 50000]
    matches = list(re.finditer(glyph_pattern, glyph_dsc_section))

    if char_index < len(matches):
        match = matches[char_index]
        glyph_info = {
            'bitmap_index': int(match.group(1)),
            'adv_w': int(match.group(2)),
            'box_w': int(match.group(3)),
            'box_h': int(match.group(4)),
            'ofs_x': int(match.group(5)),
            'ofs_y': int(match.group(6))
        }
        return bytes_data, glyph_info

    return bytes_data, None

def unpack_4bpp(bytes_data):
    """将4bpp数据解包为像素值"""
    pixels = []
    for byte_val in bytes_data:
        high_nibble = (byte_val >> 4) & 0x0F
        low_nibble = byte_val & 0x0F
        pixels.extend([high_nibble, low_nibble])
    return pixels

def visualize_bitmap(pixels, width, height):
    """可视化位图"""
    if len(pixels) < width * height:
        print(f"Warning: Not enough pixels. Expected {width * height}, got {len(pixels)}")
        return

    # 使用ASCII字符表示不同灰度
    chars = ' .:-=+*#%@'

    for y in range(height):
        line = ""
        for x in range(width):
            idx = y * width + x
            if idx < len(pixels):
                pixel = pixels[idx]
                char_idx = min(pixel * len(chars) // 16, len(chars) - 1)
                line += chars[char_idx] * 2  # 每个像素用2个字符显示
        print(line)

def compare_bitmaps(pixels1, pixels2, width, height):
    """对比两个位图的差异"""
    diff_count = 0
    max_diff = 0

    print("\nPixel-by-pixel comparison:")
    print("Position | Official | Current | Diff")
    print("-" * 45)

    for y in range(height):
        for x in range(width):
            idx = y * width + x
            if idx < len(pixels1) and idx < len(pixels2):
                p1 = pixels1[idx]
                p2 = pixels2[idx]
                diff = abs(p1 - p2)

                if diff > 0:
                    diff_count += 1
                    max_diff = max(max_diff, diff)
                    if diff_count <= 20:  # 只显示前20个差异
                        print(f"({x:2d},{y:2d})  |   {p1:2d}    |   {p2:2d}   | {diff:+3d}")

    if diff_count > 20:
        print(f"... and {diff_count - 20} more differences")

    print(f"\nTotal differences: {diff_count} / {width * height} pixels")
    print(f"Max difference: {max_diff}")
    print(f"Difference rate: {diff_count * 100.0 / (width * height):.2f}%")

# 测试字符 U+0021 "!"
print("=" * 60)
print("Comparing U+0021 '!' bitmap data")
print("=" * 60)

official_bytes, official_info = extract_glyph_bitmap(r"Z:\My_Font_1.c", 0x0021)
current_bytes, current_info = extract_glyph_bitmap(r"Z:\My_Font.c", 0x0021)

print("\nOfficial tool glyph info:")
if official_info:
    for key, value in official_info.items():
        print(f"  {key}: {value}")

print("\nCurrent tool glyph info:")
if current_info:
    for key, value in current_info.items():
        print(f"  {key}: {value}")

if official_bytes and current_bytes:
    print(f"\nOfficial bytes: {len(official_bytes)}")
    print(f"Current bytes:  {len(current_bytes)}")

    # 解包4bpp数据
    official_pixels = unpack_4bpp(official_bytes)
    current_pixels = unpack_4bpp(current_bytes)

    print(f"\nOfficial pixels: {len(official_pixels)}")
    print(f"Current pixels:  {len(current_pixels)}")

    if official_info and current_info:
        width = official_info['box_w']
        height = official_info['box_h']

        print(f"\nBitmap size: {width}x{height}")

        print("\n" + "=" * 60)
        print("Official tool bitmap:")
        print("=" * 60)
        visualize_bitmap(official_pixels, width, height)

        print("\n" + "=" * 60)
        print("Current tool bitmap:")
        print("=" * 60)
        visualize_bitmap(current_pixels, width, height)

        print("\n" + "=" * 60)
        compare_bitmaps(official_pixels, current_pixels, width, height)
        print("=" * 60)
