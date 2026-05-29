#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
全面对比两个字体文件的所有方面
"""

import re

def extract_all_glyphs(filename):
    """提取所有字符的完整信息"""
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()

    glyphs = {}

    # 提取glyph_dsc数组
    glyph_pattern = r'\{\.bitmap_index = (\d+), \.adv_w = (\d+), \.box_w = (\d+), \.box_h = (\d+), \.ofs_x = (-?\d+), \.ofs_y = (-?\d+)\}'
    glyph_dsc_start = content.find('static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[]')

    if glyph_dsc_start != -1:
        glyph_dsc_section = content[glyph_dsc_start:glyph_dsc_start + 50000]
        matches = list(re.finditer(glyph_pattern, glyph_dsc_section))

        # 跳过第一个（id=0保留）
        for i, match in enumerate(matches[1:], start=1):
            unicode_val = 0x0020 + i  # 从空格开始
            glyphs[unicode_val] = {
                'bitmap_index': int(match.group(1)),
                'adv_w': int(match.group(2)),
                'box_w': int(match.group(3)),
                'box_h': int(match.group(4)),
                'ofs_x': int(match.group(5)),
                'ofs_y': int(match.group(6))
            }

    # 提取位图数据
    bitmap_start = content.find('static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[]')
    if bitmap_start != -1:
        bitmap_end = content.find('};', bitmap_start)
        bitmap_section = content[bitmap_start:bitmap_end]

        # 按字符分割
        char_sections = re.split(r'/\* U\+([0-9A-F]{4})', bitmap_section)

        for i in range(1, len(char_sections), 2):
            unicode_val = int(char_sections[i], 16)
            data_section = char_sections[i + 1]

            # 提取十六进制数据
            hex_values = re.findall(r'0x([0-9a-fA-F]+)', data_section)
            bytes_data = [int(h, 16) for h in hex_values]

            if unicode_val in glyphs:
                glyphs[unicode_val]['bitmap_bytes'] = bytes_data

    return glyphs

def compare_glyphs(official, current, unicode_val):
    """对比单个字符的所有属性"""
    char = chr(unicode_val)
    print(f"\n{'='*70}")
    print(f"Character: U+{unicode_val:04X} '{char}'")
    print(f"{'='*70}")

    if unicode_val not in official:
        print("  Not found in official file")
        return
    if unicode_val not in current:
        print("  Not found in current file")
        return

    off = official[unicode_val]
    cur = current[unicode_val]

    # 对比所有属性
    attrs = ['bitmap_index', 'adv_w', 'box_w', 'box_h', 'ofs_x', 'ofs_y']

    print("\nGlyph Descriptor:")
    print(f"{'Attribute':<15} | {'Official':>10} | {'Current':>10} | {'Match'}")
    print("-" * 70)

    all_match = True
    for attr in attrs:
        off_val = off.get(attr, 'N/A')
        cur_val = cur.get(attr, 'N/A')
        match = "OK" if off_val == cur_val else "DIFF"
        if off_val != cur_val:
            all_match = False
        print(f"{attr:<15} | {str(off_val):>10} | {str(cur_val):>10} | {match}")

    # 对比位图数据
    if 'bitmap_bytes' in off and 'bitmap_bytes' in cur:
        off_bytes = off['bitmap_bytes']
        cur_bytes = cur['bitmap_bytes']

        print(f"\nBitmap Data:")
        print(f"  Official length: {len(off_bytes)} bytes")
        print(f"  Current length:  {len(cur_bytes)} bytes")

        if len(off_bytes) == len(cur_bytes):
            diff_count = sum(1 for i in range(len(off_bytes)) if off_bytes[i] != cur_bytes[i])
            if diff_count > 0:
                print(f"  Differences: {diff_count}/{len(off_bytes)} bytes")
                print(f"\n  First 10 differences:")
                print(f"  {'Index':<8} | {'Official':>10} | {'Current':>10} | {'Diff'}")
                print("  " + "-" * 50)

                shown = 0
                for i in range(len(off_bytes)):
                    if off_bytes[i] != cur_bytes[i] and shown < 10:
                        print(f"  {i:<8} | 0x{off_bytes[i]:02x} ({off_bytes[i]:3d}) | 0x{cur_bytes[i]:02x} ({cur_bytes[i]:3d}) | {cur_bytes[i] - off_bytes[i]:+4d}")
                        shown += 1
            else:
                print(f"  OK - All bytes match!")
        else:
            print(f"  DIFF - Length mismatch!")

    return all_match

def main():
    print("="*70)
    print("Comprehensive Font File Comparison")
    print("="*70)

    official_file = r"Z:\My_Font_1.c"
    current_file = r"Z:\My_Font.c"

    print(f"\nLoading official file: {official_file}")
    official_glyphs = extract_all_glyphs(official_file)
    print(f"  Found {len(official_glyphs)} glyphs")

    print(f"\nLoading current file: {current_file}")
    current_glyphs = extract_all_glyphs(current_file)
    print(f"  Found {len(current_glyphs)} glyphs")

    # 对比前5个字符
    test_chars = [0x0021, 0x0022, 0x0023, 0x0030, 0x0041]  # !"#0A

    for unicode_val in test_chars:
        if unicode_val in official_glyphs and unicode_val in current_glyphs:
            compare_glyphs(official_glyphs, current_glyphs, unicode_val)

    # 统计总体差异
    print("\n" + "="*70)
    print("Overall Statistics")
    print("="*70)

    total_chars = 0
    desc_match = 0
    bitmap_match = 0

    for unicode_val in official_glyphs:
        if unicode_val in current_glyphs:
            total_chars += 1

            # 检查描述符
            off = official_glyphs[unicode_val]
            cur = current_glyphs[unicode_val]

            attrs_match = all(off.get(attr) == cur.get(attr)
                            for attr in ['adv_w', 'box_w', 'box_h', 'ofs_x', 'ofs_y'])
            if attrs_match:
                desc_match += 1

            # 检查位图
            if 'bitmap_bytes' in off and 'bitmap_bytes' in cur:
                if off['bitmap_bytes'] == cur['bitmap_bytes']:
                    bitmap_match += 1

    print(f"\nTotal characters compared: {total_chars}")
    print(f"Descriptor matches: {desc_match}/{total_chars} ({desc_match*100/total_chars:.1f}%)")
    print(f"Bitmap matches: {bitmap_match}/{total_chars} ({bitmap_match*100/total_chars:.1f}%)")

if __name__ == "__main__":
    main()
